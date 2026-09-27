# Asynchronous Hider–Seeker Game
## MVP System Architecture, Game Mechanics, and NPC AI Design

**Status:** Implementation-oriented MVP proposal  
**Architecture:** Backend-authoritative, asynchronous ticks, HTTP snapshots, C++ simulation core  
**Hard capacity limits:** 1 seeker, 8 hiders, 16 servers; every bounded collection is at most 64 entries

---

## 1. Purpose and design principles

This document specifies an asynchronous hider–seeker game in which hiders manage infrastructure, pursue reward-bearing goals, and interpret evidence, while a seeker NPC investigates likely hider infrastructure.

The system is designed around these constraints:

- One seeker maximum, eight hiders maximum, and sixteen servers maximum.
- Fixed-size arrays and a singly allocated authoritative match-state block; no dynamically growing collections inside the game state.
- No mechanic, queue, history, or candidate set ever requires more than 64 entries.
- Human hiders perform their own searching and evidence analysis. The only automatic searcher is the seeker NPC.
- The backend is authoritative for actions, hidden state, seeker decisions, goal completion, resource costs, and outcomes.
- Clients may use WebAssembly (WASM) for rendering and safe previews, but clients are untrusted.
- The seeker AI and optional hider NPC AI are policies layered over deterministic rules. Neither model can mutate state or decide outcomes directly.

### Feasibility in brief

This architecture is feasible for an MVP. The difficult parts are defining engaging, counterplay-friendly signals and collecting representative training data—not model size or accelerator capacity. Start with deterministic baseline policies and a simulator. Add learned ranking only where held-out match evaluation shows a reliable improvement.

---

## 2. System architecture

### 2.1 Components

**Authoritative simulation core (C++)**  
Contains fixed-size state, deterministic game rules, action validation, tick advancement, metric changes, goal evaluation, probe resolution, event generation, and policy interfaces. The same domain source may be compiled for server and WASM, but the WASM build must not contain private backend capabilities.

**Backend tick runner**  
Loads a match revision, applies queued intents, advances due ticks, invokes the server-only seeker policy, commits the result transactionally, and schedules future work. It is the only authority for match state.

**Intent API**  
Authenticates players, validates authorization and request shape, applies idempotency and revision checks, and queues commands for backend execution.

**Persistence and event log**  
Stores versioned state, applied intents, events, ruleset/model versions, and deterministic replay information. A relational or transactional store can be used for MVP; the architecture does not require a particular database.

**Snapshot publisher and object storage/CDN**  
Creates audience-specific, sanitized immutable snapshots at revision-addressed paths. A small mutable pointer identifies the latest revision.

**Client**  
Fetches snapshots over HTTP, renders public/player-authorized views, and submits intents. Local WASM may validate form shape or preview deterministic public effects, but the backend validates every action.

**Offline simulation and AI training tools**  
Generate simulated matches, label outcomes from authoritative ground truth, train/evaluate compact policies, and export versioned artifacts. Training is not part of the match-tick critical path.

### 2.2 Security boundary

Anything delivered to a client—including WASM binaries, model weights, constants, and debug symbols—must be treated as inspectable. Compile-time separation helps prevent accidental inclusion but is not a security boundary. Keep hidden traces, true server ownership, seeker beliefs, secret thresholds, production seeker weights, and authoritative decisions on the backend.

### 2.3 Suggested code layout

```text
/engine-core
  /domain
    state.hpp
    rules.cpp
    tick.cpp
    goals.cpp
    probe_resolution.cpp
    serialization.cpp
  /seeker
    feature_schema.hpp
    feature_extract.cpp
    policy_interface.hpp
    baseline_policy.cpp
    model_policy.cpp
  /hider_agent
    observation.hpp
    goal_evaluator.cpp
    planner.cpp
    controller.cpp
    baseline_policy.cpp
  /projection
    audience_views.cpp
    serializer.cpp
  /wasm
    safe_bindings.cpp
  /server
    intent_api.cpp
    tick_worker.cpp
    persistence.cpp
    snapshot_publisher.cpp
/training
  dataset_schema.md
  simulator_export.cpp
  train_seeker_ranker.py
  train_hider_ranker.py
  evaluate_policy.py
  export_model.py
  parity_vectors/
```

---

## 3. Match model, mechanics, and state

### 3.1 Fixed-size state

The authoritative state uses predetermined arrays. The following is a design sketch; exact integer widths, padding, enum values, and ABI must be finalized before implementation.

```cpp
constexpr uint8_t MAX_HIDERS = 8;
constexpr uint8_t MAX_SERVERS = 16;
constexpr uint8_t MAX_GOALS = 16;
constexpr uint8_t MAX_REQUIREMENTS_PER_GOAL = 16;
constexpr uint8_t MAX_EVENTS = 64;
constexpr uint8_t HISTORY_TICKS = 16;
constexpr uint8_t MAX_INTENTS_PER_TICK = 64;
constexpr uint8_t MAX_SEEKER_TARGETS = 16;
constexpr uint8_t MAX_HIDER_PLAN_STEPS = 16;

struct MetricFrame {
    uint8_t cpu, memory, disk, network;
    uint8_t heat, errors, users;
};

struct ServerState {
    uint8_t active;
    uint8_t owner_hider_id;    // private; 0xFF means unowned
    uint8_t public_flags;
    uint8_t trace_strength;    // private
    uint8_t trace_age;         // private
    MetricFrame current;
    MetricFrame history[HISTORY_TICKS];
    uint8_t history_head;
};

struct HiderState {
    uint8_t active;
    uint8_t status;
    uint8_t action_points;
    uint8_t server_id;
    uint16_t resources[RESOURCE_TYPE_COUNT];
    uint8_t goal_progress[MAX_GOALS];
};

struct SeekerState {
    uint8_t connection_tokens;
    uint8_t max_tokens;
    uint8_t heat;
    uint8_t belief[MAX_SERVERS];
    uint8_t target_ids[MAX_SEEKER_TARGETS];
    uint8_t target_count;
    uint8_t target_head;
    uint8_t last_probe_tick[MAX_SERVERS];
};

struct MatchState {
    uint32_t schema_version;
    uint32_t ruleset_version;
    uint64_t revision;
    uint32_t tick;
    uint64_t rng_state;
    uint8_t phase;
    uint8_t hider_count;
    uint8_t server_count;
    HiderState hiders[MAX_HIDERS];
    ServerState servers[MAX_SERVERS];
    SeekerState seeker;
    GoalState goals[MAX_GOALS];
    Event events[MAX_EVENTS];
    uint8_t event_head;
    uint8_t event_count;
};
```

`RESOURCE_TYPE_COUNT` is a compile-time bounded enum count, never above 64. The Letter of Authorization is one resource type. Store no pointers, strings, vectors, or owning containers inside the state block. Use IDs and fixed arrays. Serialize through an explicit versioned format; do not persist raw compiler memory layout. Check all array counts and IDs before indexing, use saturating arithmetic, and make migration rules explicit.

### 3.2 Tick lifecycle

Ticks represent game intervals, not necessarily wall-clock seconds. The backend advances a tick when its scheduled deadline arrives or a game rule closes the current action window. A tick proceeds in this order:

1. Lock/read the current match revision and determine eligible intents.
2. Validate and order accepted hider intents using stable server receipt sequence and deterministic tie-breakers.
3. Apply legal hider actions and immediate costs atomically.
4. Advance server metrics, trace aging, temporary effects, and resource regeneration.
5. Derive the seeker-visible evidence for this tick.
6. Let the seeker policy rank legal targets and execute probes up to its token/probe limit.
7. Resolve probe outcomes, capture/escape, goal qualification, rewards, and end conditions.
8. Persist next state, consumed intent IDs, decisions, and events as a single committed revision.
9. Publish audience-specific immutable snapshots for that revision.
10. Schedule the next tick.

The ordering of simultaneous actions must be part of the ruleset. A retry must not apply an intent twice. Token exhaustion does not trigger an implicit tick; callers receive an explicit rejection or queue status.

### 3.3 Metrics and randomness

Server metrics can include CPU, memory, disk, network, errors, users, and heat. Define ranges (for example, percentages 0–100), units, update equations, clamping, and visibility per field. Hiders act through named modes with server-defined costs and effects. For example, suppressing errors may lower visible error growth while increasing a hidden risk trace; boosting CPU may raise capacity and load; rotating routes may transfer activity while leaving a decaying trace.

Use integer/fixed-point arithmetic for reproducibility. Randomness comes from a server-held match seed and deterministic counter/tick sequence. Never distribute future random values to clients. Record enough information to replay every result.

### 3.4 Goals and rewards

Goals are versioned, machine-readable definitions. A goal specifies visibility, active window, reward type/amount, requirements, costs, completion action, expiration, and failure/cancellation behavior. Completion and reward grants are exclusively determined by the backend goal resolver.

The example interface is:

```cpp
enum RewardType : uint8_t { REWARD_NONE, REWARD };
enum RequirementType : uint8_t {
    REQ_RESOURCE_MIN,
    REQ_RESOURCE_SPEND,
    REQ_ACTIVE_SERVER_COUNT,
    REQ_SERVER_METRIC_THRESHOLD,
    REQ_ACTION_COUNT,
    REQ_CONFIGURED_SERVER,
    REQ_CUSTOM_RULE
};

struct Requirement {
    uint8_t type;
    uint8_t subject_id;
    uint16_t amount;
    uint16_t threshold;
};

struct GoalDefinition {
    uint8_t active;
    uint8_t goal_id;
    uint8_t status;
    uint8_t reward_type;
    uint16_t reward_amount;
    uint8_t requirement_count; // <= MAX_REQUIREMENTS_PER_GOAL
    Requirement requirements[MAX_REQUIREMENTS_PER_GOAL];
    uint8_t completion_action;
    uint16_t deadline_tick;
};
```

Narrative-only or unstructured requirements are not automatically plannable until given a deterministic evaluator and action mapping.

### 3.5 Hider actions and player analysis

Players inspect permitted metrics, history, and events themselves. The human client should not automatically find suspicious servers, choose search targets, or summarize evidence in a way that removes the intended analysis mechanic. The NPC seeker is the sole automated searcher.

Hider actions are explicit commands, such as configure server, suppress errors, boost CPU, rotate route, pay a goal, add a server, or remove a server. Clients submit the desired action only. The backend derives cost and effects from the ruleset.

---

## 4. Resources and server lifecycle

### 4.1 Hider action points

Action points are authoritative per-hider resources. They regenerate according to the ruleset, within a fixed cap. The client never submits the cost. The backend validates and deducts points only when an action is accepted and applied.

### 4.2 Letter of Authorization (LOA)

The **Letter of Authorization** is a spendable hider resource that permits adding or removing one server entirely.

- `ADD_SERVER` costs 1 LOA under the initial ruleset and activates an available slot, subject to the 16-server maximum.
- `REMOVE_SERVER` costs 1 LOA and decommissions an active slot.
- If a hider occupies the server being removed, the hider must relocate as part of the same atomic action or removal is rejected.
- Goals, pending intents, and seeker targets referencing a removed server follow explicit ruleset behavior (cancel, rebind, or invalidate); never leave dangling references.
- IDs are stable slots 0–15. Reusing a slot requires clearing its metrics, history, ownership, traces, and stale references.
- Rejected actions spend no LOA. Accepted actions spend the LOA atomically with the lifecycle change.
- Public lifecycle events are shown only when intended by the game rules and must not disclose hidden occupancy.

The hider AI planner treats LOAs as scarce prerequisites and plans within capacity. The seeker policy masks inactive slots and resets slot features/history when a slot is reused.

### 4.3 Seeker connection tokens

The seeker holds up to a configured maximum (for example, 8) connection tokens. Prefer simple fixed regeneration for MVP, such as 2 tokens per tick up to the cap. If heat modifies regeneration, specify the bounded formula in the ruleset. Unspent-token carryover must be explicitly defined.

Each probe has a known cost, usually 1 token, paid before the result. A successful, inconclusive, or empty probe generally costs the same unless a visible rule states otherwise. The target-selection policy does not choose token regeneration or probe outcome.

### 4.4 FIFO and history semantics

A stack is LIFO; a FIFO is a queue. Use a fixed ring buffer with `head` and `count` for pending targets. When full, define an explicit deterministic policy, such as retaining the highest-priority candidates or evicting the oldest pending target. Never silently overwrite a target that may affect outcomes. Metric history is also a ring buffer; overwriting its oldest frame is expected and specified.

---

## 5. API and snapshot protocol

### 5.1 Submit intent

```http
POST /api/v1/matches/{match_id}/intents
Authorization: Bearer <session>
Content-Type: application/json
```

```json
{
  "expected_revision": 104,
  "idempotency_key": "4d98b2b5-...",
  "intent": {
    "type": "CONFIGURE_SERVER",
    "server_id": 4,
    "mode": "SUPPRESS_ERROR",
    "goal_id": 2
  }
}
```

Identity comes from authentication, never from a body-supplied `client_id`. The request omits action cost, metric deltas, success result, reward amount, and hidden state. `ADD_SERVER` and `REMOVE_SERVER` carry only the necessary slot and relocation target. The backend derives LOA cost.

Example response:

```json
{
  "intent_id": "in_...",
  "accepted": true,
  "status": "QUEUED",
  "accepted_against_revision": 104
}
```

Statuses include `QUEUED`, `APPLIED`, and `REJECTED`. Idempotency keys make retries safe. Revision conflicts return a conflict and the latest authorized revision. Validation never advances the tick as a side effect.

### 5.2 Read audience-specific snapshot

```http
GET /api/v1/matches/{match_id}/views/{audience}/latest
```

Audience is authenticated and authorized by the backend; a client cannot request another role's view just by changing a path parameter. A hider view may include that hider's private resources and LOA count, public server metrics/history/events, and visible goals. It omits other hiders' private resources, hidden ownership, raw traces, seeker beliefs, and model scores.

Example abbreviated view:

```json
{
  "match_id": "match_alpha",
  "revision": 105,
  "tick": 105,
  "ruleset_version": 3,
  "view_version": 2,
  "phase": "ACTIVE",
  "self": {
    "hider_id": 2,
    "action_points": 2,
    "resources": {"credits": 50, "intel": 2, "loa": 1}
  },
  "servers": [{
    "server_id": 4,
    "metrics": {"cpu": 45, "memory": 60, "disk": 20,
                 "network": 80, "errors": 2, "heat": 15},
    "history": []
  }],
  "goals": [{
    "goal_id": 2,
    "status": "IN_PROGRESS",
    "reward_type": "REWARD",
    "reward_amount": 1,
    "requirements": []
  }],
  "events": []
}
```

### 5.3 Immutable snapshots

Store snapshots under immutable revisioned keys, for example:

```text
matches/{match_id}/views/{audience}/rev-{revision}.json
```

After the state transaction commits, write the revision snapshot and update a small mutable `latest` pointer or metadata row atomically where possible. Do not overwrite a stable `latest.json` object and call it immutable. Revision URLs can have long CDN cache lifetimes; the pointer gets a short TTL or ETag validation. Include content hash, schema version, view version, and ruleset version.

Optional endpoints:

- `GET /api/v1/matches/{id}` — authorized metadata and current revision
- `GET /api/v1/matches/{id}/intents/{intent_id}` — command status
- `GET /api/v1/matches/{id}/events?after_revision=N` — authorized event stream by polling
- `POST /api/v1/matches/{id}/concede` — explicit concession

No WebSocket connection is required. Polling and optional out-of-band notifications fit the asynchronous design.

---

## 6. Seeker NPC policy and probe resolution

### 6.1 Separation of policy and rules

The seeker policy chooses where to spend investigation effort. The deterministic resolver determines what a probe finds and what happens next.

```text
Seeker-visible evidence + seeker memory
                  ↓
          target-ranking policy
                  ↓
      legal-target/token validation
                  ↓
    hidden-state probe resolution
                  ↓
            event/state update
```

The selector receives only seeker-permitted observations. The resolver may inspect hidden traces after selection. A hit may expose evidence, force relocation, or support capture according to explicit rules. Define consequences and counterplay in the ruleset; do not make success a hidden, model-controlled judgment.

### 6.2 Deterministic responsibilities

Keep deterministic:

- Hider intent validation/costs and metric/trace updates
- Seeker token accounting and legal target mask
- Probe cost and result calculation given state and seeded randomness
- Capture, escape, goal, and reward rules
- Tie-breaking and candidate truncation
- Queue behavior and fallback

### 6.3 What the seeker can learn

A compact ranker can learn which combinations and time patterns of seeker-visible evidence predict useful probes. Per-server inputs may include:

- Normalized current metrics and deltas/trends over 1, 2, 4, and 8 ticks
- Public event category counts
- Time since last probe and prior public probe outcome category
- Seeker's own bounded belief score and recent target history
- Goal activity visible to seeker
- Tick phase and remaining match duration
- Active/legal candidate mask

Do not include true ownership, hidden trace strength, future events, post-probe outcomes for the current decision, or any privileged label. This prevents leakage that would make offline accuracy unrealistically high.

### 6.4 Candidate limit and policy output

At most 16 server candidates are ranked. The seeker can execute at most 8 probes per tick under the example token cap. Policy outputs are scores or an ordered list, not authoritative token costs, hidden truth, or success decisions. The backend masks illegal/inactive targets and applies deterministic tie-breaking.

---

## 7. Seeker AI data, training, and deployment pipeline

### 7.1 Simulator-first data strategy

Early real player data will be sparse, correlated, and shaped by one particular NPC policy. Start with a simulator using the authoritative rules and varied scripted hider styles:

- Random legal behavior
- Goal-driven behavior
- Conservative stealth
- Aggressive resource farming
- Reactive evasion after probes
- Adversarial policies designed to exploit obvious scoring rules

Generate matches across seeds, configurations, player counts, and seeker policies. Use whole-match and held-out policy-family splits, not random tick splits. Adjacent ticks from a match are not independent samples.

### 7.2 Ground truth and labels

Record separate labels:

**Outcome labels:** whether a probe found a hider or relevant trace at that instant, produced actionable evidence, led to capture or forced relocation within a defined horizon, and its token/heat cost. These come only from authoritative simulation state and rules.

**Policy labels:** teacher/baseline target choices and, if affordable, counterfactual target values estimated by repeated seeded rollouts.

A target is not automatically “good” just because a hider is there. Define utility with capture, information, forced-relocation value, token cost, heat cost, and timing. Version the weights. Keep hidden ground-truth fields in a restricted label partition, never the inference inputs.

### 7.3 Training record and feature parity

Each record contains match/tick/revision, ruleset and feature versions, simulation seed class, hider strategy family, seeker-visible feature tensor, legal mask, available tokens, chosen target, probe outcome, future horizon outcome, policy/model version, and data provenance. Avoid unnecessary personal data.

Use the same feature contract and test vectors offline and in production. Document units, ranges, clipping, missing values, normalization, and inactive-slot masking. Verify feature parity byte-for-byte or within a defined quantization tolerance.

### 7.4 Model architecture

Begin with a small shared scorer:

```text
server features [16, F] ─┐
global context [G] ───────┼→ shared scorer → 16 scores
legal mask [16] ─────────┘
```

A compact MLP with hidden sizes around 32 then 16 and one score per server is a reasonable starting point. Use an optional explicit `WAIT` candidate only if waiting is a real strategic action. Start without recurrent networks: seeker memory can be a small explicit fixed array/state field. Train as ranking or policy classification; alternatively predict calibrated outcomes but retain deterministic cost/value selection.

### 7.5 DGX Spark / GB10 workflow

DGX Spark/GB10 is appropriate for local experimentation and batch training, but production inference does not need the accelerator for 16 candidates and a small MLP. Benchmark the actual software stack rather than assume capacity.

1. Generate bounded simulator episodes on CPU in parallel using deterministic seeds.
2. Write compact columnar shards and separate observation features from protected labels.
3. Inspect class/strategy/goal coverage before training.
4. Train small candidates in PyTorch or an equivalent framework; record dataset hash, code commit, schema/ruleset versions, hyperparameters, and seeds.
5. Run multiple training seeds and compare against the deterministic baseline.
6. Export to ONNX or a documented small inference format.
7. Test exported inference and feature extraction against frozen vectors.
8. Benchmark CPU inference in the backend; quantize to INT8 only if size/latency benefit justifies any quality loss.
9. Sign and checksum the artifact and its manifest.

Do not download model weights from GitHub during a tick. Use a controlled artifact store. Treat public model weights as disclosed policy and do not distribute private seeker weights to clients.

### 7.6 Evaluation and promotion

Measure capture rate, time-to-capture, probe precision/recall, wasted-token rate, target diversity, performance by hider style/player count, calibration/ranking quality, latency, memory, and replay determinism. Game balance matters alongside predictive metrics: a capture-rate improvement that makes a hider strategy unplayable is a regression.

Promote only after whole-match held-out evaluation, baseline comparison, balance-band checks, and parity checks. Run shadow scoring first, then a limited server-side canary. Keep atomic rollback and record policy/model version per decision.

---

## 8. Seeker deterministic baseline and fallback

Use a transparent server-only scoring policy as the initial agent and fallback. Example illustrative score:

```cpp
int32_t score_server(const Features& f) {
    return 3 * f.error_delta_2
         + 2 * f.network_delta_2
         + 2 * f.heat_delta_2
         + 1 * f.cpu_delta_2
         + f.trace_event_count
         - f.recent_probe_penalty;
}
```

Clamp inputs and score, ignore inactive candidates, and use a stable or seeded deterministic tie-break. Tune coefficients through simulation; these are not validated balance values.

Fall back if the model is missing, incompatible, fails signature/checksum, feature extraction fails, inference exceeds its budget, or output is invalid/illegal. Record fallback reason internally. Do not switch policy unpredictably mid-tick; persist the policy version used for the decision.

---

## 9. Hider NPC AI for reward-bearing goals

### 9.1 Objective and feasibility

The hider NPC seeks any visible active goal whose authoritative definition awards positive `REWARD`. It needs no separate model for each goal when goals use a common schema and supported requirement types. New goal mechanics need deterministic evaluator and planner support before the agent can use them.

Candidate goal eligibility:

1. Visible to this hider.
2. Active and not expired or completed.
3. Reward type is `REWARD` and reward amount is positive.
4. All requirement types have evaluator/planner support.
5. A feasible action plan appears possible given resources, action points, LOAs, server capacity, phase, and deadline.

The agent estimates feasibility; only the backend goal resolver grants rewards.

### 9.2 Observation boundary

The hider AI receives exactly the authorized hider view: its own resources/action points/LOAs, visible goals, permitted public server metrics/history/events, its own known actions, and outcomes of prior intents. It never receives hidden traces, seeker belief state, seeker model scores, other hiders' private resources, hidden ownership, future randomness, or inaccessible backend state.

It outputs proposed intents only. Normal validation, authorization, revision, idempotency, and rate limits apply just as for a human hider.

### 9.3 Decision pipeline

```text
Authorized observation
        ↓
Version check and bounded feature extraction
        ↓
Enumerate visible positive-REWARD goals
        ↓
Evaluate requirements, cost, time, LOA and capacity
        ↓
Select or continue a goal plan
        ↓
Choose one legal action
        ↓
Submit intent to backend
        ↓
Reconcile result and replan
```

For each goal, deterministically estimate requirements satisfied, prerequisite actions, resources/action points/ticks needed, LOA/server lifecycle needs, deadline feasibility, and exposure cost if a calibrated authorized estimate exists. Mark uncertainty rather than treating estimates as guarantees.

Rank feasible goals using a transparent utility:

```text
goal utility = expected REWARD value
             + useful intermediate progress
             - resource/action-point cost
             - estimated completion delay
             - estimated exposure/capture risk
             - risk of becoming infeasible
```

Weights start as designer-authored configuration and are tuned in simulation/playtests. Prevent thrashing with commitment duration or a switching threshold. Switch when the selected goal is complete, expired, infeasible, or a rival candidate is materially better for a configured period.

### 9.4 Bounded plan and behavior controller

Use fixed action templates and a maximum plan size such as 16. Example: acquire resources → spend an LOA to add a server if required → configure it → meet goal conditions → submit completion action. Revalidate before each action and replan after rejection, probe consequence, server loss, resource/goal change, or ruleset transition.

A small finite-state controller manages durable workflow:

| State | Purpose |
|---|---|
| `ASSESS` | Refresh authorized observation and validate prior intent |
| `PLAN` | Enumerate feasible reward goals and form a plan |
| `EXECUTE` | Submit one legal next action |
| `RECOVER` | Correct for rejection or changed state |
| `RESELECT` | Abandon complete/expired/infeasible goal and select again |

Persist selected goal, bounded plan step/version, and outstanding intent ID. Reconstruct other state from the latest view.

### 9.5 Hider AI training pipeline

Start with deterministic goal evaluation and planning. Train only after simulations reveal measurable baseline weaknesses.

**Data generation:** Use varied scripted hiders and seekers, including random legal, cheapest feasible goal, reward-per-action, low-exposure, aggressive reward pursuit, resource/LOA-aware, and reactive behaviors. Vary goals, configurations, seeds, and opponent policies.

**Ground truth:** The authoritative simulator labels goal eligibility, feasibility, completion, expiration, invalidation, reward grant, resources/LOAs/action points/ticks consumed, capture/survival, and resulting state. Training-only hidden fields remain separate from observation features.

**Value target:** One possible target is reward earned within a specified horizon minus versioned resource/action costs and failure/capture costs. Always report raw outcomes as well as any weighted score.

**Training records:** Include observation features, eligible goal list, feasibility estimates, legal action mask, selected goal/action, policy identity, version metadata, and subsequent authoritative outcomes. Split by complete match and held-out hider/seeker policy families.

**Features:** Use identical versioned feature extraction offline and in production. Exclude hidden seeker state, true traces, future events, and outcome labels. Keep label fields in a separate schema.

**Model:** A compact shared goal scorer can use reward amount/type, requirement representation, estimated cost, deadline, feasibility, and permitted public risk features. Optionally add a small legal-action ranker. A small gradient-boosted model or MLP is sufficient to start. Mask ineligible goals. The model ranks only; deterministic code enforces requirements, budgets, LOA usage, and action legality.

**Evaluation:** Compare with deterministic baseline on held-out complete matches/maps/configurations and policy families. Track reward per match, reward goals completed, time to first reward, reward per action point/resource, capture/survival, invalid intents, replan frequency, LOA efficiency, and results per goal type. Require balance bands and confidence intervals over independent matches.

**Deployment:** Use server-side inference, versioned features/rules/goals/model, signed artifacts, offline loading, parity vectors, shadow mode, canary, monitoring, and immediate rollback. Fall back to baseline on model or feature failures. The DGX Spark/GB10 can run simulator training, but a tiny production ranker should run on backend CPU.

---

## 10. Transactionality, replay, and snapshot publication

Treat every tick as a transaction:

1. Load state at revision `r`.
2. Verify expected revision and acquire a lock or compare-and-swap token.
3. Apply a bounded batch of intents and deterministic tick logic in memory.
4. Persist next state, consumed intent IDs, events, policy versions, and replay data atomically as revision `r+1`.
5. Generate sanitized snapshots from committed state.
6. Publish immutable revision objects, then update the latest pointer.

Durable scheduling is still required even without sockets. A worker/job queue must find due matches and retry safely. Persist action logs, initial seed, ruleset version, seeker/hider policy versions, and nondeterministic inputs so decisions can be replayed or explained.

---

## 11. Anti-cheating and security

- Authenticate every command; derive player identity from credentials.
- Authorize match and audience access for every request.
- Validate all IDs, counts, enums, ranges, costs, phases, and transitions server-side.
- Keep hidden data out of snapshots, errors, timing signals, WASM, debug endpoints, and public artifacts.
- Treat WASM as an untrusted renderer; local results are previews only.
- Keep RNG state and future random draws private.
- Use revision checks, idempotency keys, request size limits, and rate limits.
- Sign model artifacts; restrict artifact-store access; never accept client-supplied model weights or NPC decisions as authoritative.
- Avoid public metrics so precise that they trivially reveal hidden occupancy unless that is an intentional mechanic.
- Audit administrative access and policy promotions.
- The hider NPC must use the same hider-authorized observation and action limits as a human. The seeker NPC must use only its explicitly defined seeker observations for target selection.

---

## 12. Observability and operations

Collect structured, revision/tick-keyed metrics for:

- Tick duration, queue delay, worker retry, transaction conflict, and stalled-match rate
- Intents accepted/rejected by reason and duplicate retry count
- Snapshot generation/publication lag, size, and cache behavior
- Seeker tokens, probe count, hit/evidence rate, target diversity, model latency, invalid outputs, and fallback rate
- Hider AI goal selections, reward outcomes, goal switches, plan recoveries, intent rejection, LOA use, model latency, and fallback rate
- Match outcomes by ruleset, goal template, and policy/model version

Use pseudonymous match IDs and avoid unnecessary personal data. Alert on stalled ticks, publication lag, repeated transaction conflicts, invalid model outputs, version mismatches, excessive fallbacks, and repeated invalid AI intents. Keep private diagnostics out of player-facing projections.

---

## 13. Implementation roadmap

1. Freeze the rules: phases, actions, metric formulas, costs, probe consequences, goal/reward contract, LOA lifecycle, and victory conditions.
2. Implement fixed-size state, bounded rings, explicit serialization, and schema migrations.
3. Implement deterministic tick engine, resource accounting, server lifecycle, goal resolver, probe resolver, and replay.
4. Implement authenticated intent API with idempotency and revision checks.
5. Implement audience projection and immutable revision snapshots; verify no hidden-field leakage.
6. Build player analysis UI over permitted raw evidence without automatic hider searching.
7. Implement seeker deterministic baseline, candidate mask, tokens, and probe resolution.
8. Implement hider deterministic goal evaluator, planner, behavior controller, and intent recovery.
9. Build varied simulator policies, dataset schemas, labels, parity tests, and baseline evaluation.
10. Train compact models only where baseline results identify a concrete weakness.
11. Add signed artifact loading, shadow scoring, canary, monitoring, rollback, and policy versioning.
12. Tune signals, goals, rewards, and seeker/hider balance through playtests and held-out simulations.

---

## 14. Key design decisions and limitations

- **Seeker detection is a ranking problem over at most 16 candidates.** A small learned model is practical, but useful labels and adaptive evaluation are the hard parts.
- **Rules decide outcomes.** Models choose targets or goals/actions; deterministic code validates actions, spends resources, resolves probes, completes goals, and grants rewards.
- **The hider agent can pursue any reward goal expressible in the shared schema.** Novel requirement types need evaluator and planner support; narrative conditions are not automatically understood.
- **One shared hider model is enough initially.** Multiple neural networks are not required. A finite-state controller provides workflow continuity; a learned ranker can be added as a decision aid.
- **Player analysis stays player-driven.** Do not expose automatic hider search or target recommendations to human clients if searching is intended gameplay.
- **Static snapshots do not remove backend scheduling or transactional requirements.** They remove persistent client connections, not server work.
- **Fixed arrays do not mean raw struct persistence.** Use explicit, versioned serialization and bounded buffers.
- **Immutable snapshots need immutable revision paths.** A mutable latest pointer is separate from those objects.
- **WASM separation is not secrecy.** Anything shipped to clients is inspectable.
- **GitHub is not inherently private model distribution.** Use controlled storage and signature verification; assume public weights reveal the policy.

