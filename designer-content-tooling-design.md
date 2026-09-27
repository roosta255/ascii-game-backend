# Designer Content Tooling for the Hider–Seeker Game

## Purpose

This document proposes a designer workflow and content model for authoring balanced actions, resources, server types, goals, and NPC behaviors in the asynchronous hider–seeker game.

The game’s design dependencies run in this order:

```text
Goals define desired outcomes
        ↓
Requirements define what must become true
        ↓
Actions change metrics, resources, and infrastructure
        ↓
Server types shape which actions and plans are effective
        ↓
NPC behavior chooses among a restricted set of the same actions
```

The tooling should make this chain explicit so designers can author and balance complete plans rather than isolated enum values.

## 1. Hard design constraints

- At most 1 seeker, 8 hiders, and 16 servers.
- Game state uses singly allocated fixed-size storage and predetermined arrays.
- No mechanic, queue, history, candidate set, or authored collection may require more than 64 entries.
- Human hiders interpret their own evidence. The seeker NPC is the only automatic searching agent.
- The backend validates actions, applies costs/effects, resolves probes, completes goals, and grants rewards.
- NPCs use the same action definitions as players, subject to explicit capability and observation limits.

## 2. Designer-facing tools

The first version should provide four connected tools:

1. **Content catalog editor** — Define resources, metrics, server types, actions, goals, events, and NPC behavior profiles.
2. **Rules validator/compiler** — Check references, bounds, conflicting rules, impossible goals, and fixed-capacity limits; compile a versioned game ruleset.
3. **Goal and action simulator** — Run a goal through possible action sequences and show required resources, turns, server types, and blockers.
4. **Balance workbench** — Run batches of simulated matches and compare reward completion, action usage, server choices, seeker pressure, and hider survival across ruleset versions.

A visual graph editor can follow later. Initially, structured forms with a readable YAML/JSON preview and useful validation messages are easier to build, review, and version-control.

## 3. Content model

Use first-class definitions for the concepts below. Enums provide stable identifiers; definitions hold parameters and behavior.

| Definition | Purpose |
|---|---|
| Resource | What an entity can own, earn, or spend |
| Metric | A measured property that actions or events can change |
| Server type | Infrastructure capabilities, restrictions, starting state, and tradeoffs |
| Action | An attempted operation with preconditions, cost, and effects |
| Goal | Conditions to satisfy and rewards to grant |
| Behavior profile | NPC action permissions, observations, and decision settings |
| Rule | General constraints and tick-triggered behavior |
| Event | What happened, who can observe it, and what public data it exposes |

Avoid placing all game behavior into `Trait.enum` or `Rule.enum`. Enums identify concepts. Typed definitions configure them.

## 4. Resources and metrics

### Resources

Each resource definition should specify:

- Stable ID and display name
- Ownership scope: per hider, per server, or global
- Initial amount and maximum amount
- How it is gained and spent
- Visibility and transfer/loss rules

### Metrics

Each metric definition should specify:

- Stable ID, display name, minimum, and maximum
- Starting value by server type
- Visibility to hiders and seeker
- Actions, server types, and tick rules that can change it
- Clamp behavior and per-tick decay
- History depth available to each audience

Updates must be deterministic and bounded. For example, CPU may range from 0 to 100; an effect adding 12 cannot push it above 100.

## 5. Server types

A server type should create strategic tradeoffs through capabilities and costs, not merely cosmetic labels.

Specify:

- Initial metric profile
- Actions enabled, disabled, or modified
- Resource production or upkeep
- Capacity or risk modifiers
- Goal requirements for which the type qualifies
- Public type-specific signals
- Optional seeker-probe modifiers

Example content:

```yaml
server_type:
  id: relay
  display_name: Relay
  starting_metrics:
    cpu: 20
    memory: 35
    network: 70
    heat: 5
  enabled_actions: [route_traffic, suppress_error, harvest_credits]
  disabled_actions: [deep_compute]
  action_modifiers:
    route_traffic:
      network_delta: 8
      heat_delta: 2
  upkeep:
    credits_per_tick: 1
  seeker_profile:
    probe_difficulty: 1
```

These are illustrative starting values, not validated balance values. Keep tuning values in a versioned ruleset.

## 6. Actions

Define an action once, then grant or restrict it by role, server type, trait, and NPC behavior profile.

Each action needs:

- Allowed actors: human hider, hider NPC, seeker NPC, or system
- Valid phase and target type
- Preconditions
- Resource and action-point costs
- Server-type requirements or modifiers
- Immediate and delayed effects
- Effect visibility and resulting event
- Failure outcome and whether failure consumes anything
- Optional cooldown or frequency cap

Example:

```yaml
action:
  id: harvest_credits
  actor_roles: [hider, hider_npc]
  target: connected_server
  action_point_cost: 1
  resource_cost: {}
  preconditions:
    - server_type_in: [relay, compute]
  effects:
    - resource_delta:
        resource: credits
        amount: 2
    - metric_delta:
        metric: heat
        amount: 3
  failure:
    consumes_action_points: false
  event:
    id: harvest_activity
    audience: public
```

Clients submit the action intent, not the claimed cost, effects, or result. The backend resolves those from the active ruleset.

### 6.1 Initial action families

- **Configure:** Change a mode or setpoint, usually trading one metric for another.
- **Produce:** Gain a resource, often with a metric cost or visible signal.
- **Stabilize:** Reduce a harmful metric, possibly consuming a resource.
- **Route or relocate:** Change which server handles activity, potentially leaving a trace.
- **Add/remove server:** Spend an LOA to change the infrastructure set.
- **Complete goal:** Submit a payment or qualification action.
- **Wait:** Pass or preserve an opportunity when timing makes it useful.

Avoid adding many actions that differ only in name. Each action should enable a distinct plan or tradeoff.

## 7. Goals

Goals declare conditions; they do not prescribe an AI action sequence. The evaluator checks conditions, and a planner chooses actions to satisfy them.

Start with composable requirement types:

- Own at least `N` units of a resource
- Spend at least `N` units of a resource
- Be connected to a server of type `T`
- Have a metric within `[min, max]` on a connected server
- Have at least `N` active servers
- Perform a specified action
- Satisfy an “all of” or “any of” group of other requirements

Each goal also specifies reward, visibility, active period, completion method, and expiration behavior.

```yaml
goal:
  id: low_latency_relay
  reward:
    type: REWARD
    amount: 3
  visible_to: hiders
  active_window_ticks: 8
  requirements:
    all:
      - connected_server_type: relay
      - metric_range:
          metric: network
          min: 60
          max: 80
  completion:
    action: submit_goal
  expiration: fail_without_reward
```

The hider AI can pursue any goal represented by supported requirement types. A new requirement type needs an evaluator and planner support before the AI can handle it. Narrative-only requirements need a deterministic evaluator first. Goal counts and requirement counts remain within fixed capacities.

## 8. NPC behavior profiles

NPCs use the shared action catalog, with explicit restrictions on actions and observations. An NPC profile should specify:

- Allowed and unavailable actions
- Resource and action-point limits
- Observation profile and missing information
- Target-selection method
- Behavioral priorities and commitment duration
- Bounded randomness or imperfect decision settings
- Whether memory persists across ticks

“Incomplete” behavior should arise from clear limitations—restricted actions, limited observation, or noisy choices—not hidden arbitrary failure.

```yaml
npc_profile:
  id: novice_hider
  allowed_actions: [harvest_credits, configure_relay, submit_goal]
  observation_profile: hider_standard
  action_point_cap: 2
  decision:
    policy: goal_utility_v1
    commitment_ticks: 2
    random_tie_break: 0.1
```

For the seeker, policy selects a target; deterministic rules charge tokens and resolve the probe. For a hider NPC, policy selects goals and actions; deterministic rules validate costs and grant rewards.

## 9. How this fits the existing ASCII-dungeon concepts

- **`Role.enum`** — Entity function and baseline permissions: hider, seeker, spectator/system.
- **`Trait.enum`** — Reusable modifier or capability: stealthy, noisy, resource-efficient, limited-action-set.
- **`Action.enum`** — Explicit attempted operation with preconditions, cost, and effects.
- **`Rule.enum`** — Shared constraint or trigger: token regeneration, tick ordering, server cap, metric decay.
- **`Door.enum`** — Keep for dungeon/map traversal. Model infrastructure links as typed routes/connections rather than overloading doors.
- **Add first-class types** — Resource, Metric, ServerType, Goal, Event, and BehaviorProfile.

Traits may modify actions or observations, but should not contain an entire action implementation. Rules may reference action/effect IDs, but should not duplicate action formulas.

## 10. Ruleset validation and compilation

The validator/compiler should reject or warn about:

- Unknown IDs or unsupported action/requirement types
- Goals with no reward or no possible completion path
- Action costs beyond resource or action-point caps
- Metric effects outside declared ranges without clamp behavior
- Server types with no usable actions or meaningful purpose
- NPC profiles that refer to undefined or forbidden actions
- Goals that require more servers/resources than can exist
- Any authored collection or capacity above 64
- Effects with undefined ordering or conflicting writes
- Public events that disclose fields marked private
- Changes that require a save migration without a ruleset version bump

Compilation emits a deterministic, versioned ruleset and a report of capacities, supported goal plans, unreachable goals, and migration compatibility. Existing matches stay pinned to their original ruleset unless explicitly migrated.

## 11. Goal/action simulator and balance workbench

Designers need to inspect consequences before shipping changes. The workbench should provide:

- **Goal feasibility:** Example plans, required actions/resources/LOAs, estimated ticks, and blockers.
- **Action effects:** Immediate/delayed metric and resource changes, clamps, visibility, and server-type modifiers.
- **Server comparison:** Type capabilities, starting metrics, upkeep, and goal coverage.
- **NPC matchups:** Simulations across hider and seeker profiles, server types, and goal sets.
- **Batch reports:** Reward completion, completion time, resource income/spending, server-type pick rate, capture/survival, probe efficiency, and action frequency.
- **Ruleset comparison:** Run identical seeds and behavior profiles against two versions.

Include scripted policies before ML: random legal actions, greedy reward-per-cost, conservative risk management, and reactive evasion. They expose degenerate mechanics and generate diverse simulation data.

## 12. Recommended implementation sequence

1. Add first-class content definitions for resources, metrics, server types, goals, events, and NPC profiles.
2. Define a small action/effect vocabulary and deterministic evaluator.
3. Create a few server types with meaningful differences in action access, costs, and tradeoffs.
4. Create reward goals using supported composable requirements.
5. Implement deterministic hider goal planning and seeker target-selection baselines.
6. Build compiler validation and goal-feasibility reporting.
7. Build the simulator and balance reports.
8. Tune action economy and rewards with scripted NPCs and playtests.
9. Collect simulation data and evaluate learned NPC policies only after baselines exist.

## 13. Design principle

Designers specify **what exists, what it costs, and what it changes**. Shared game code specifies **how those definitions execute**. The workbench then evaluates the full chain—goal → requirements → actions → server capabilities → costs and side effects—instead of balancing isolated enum values.
