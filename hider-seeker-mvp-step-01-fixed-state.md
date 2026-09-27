# Hider-Seeker MVP — Step 1: Freeze the rules & fixed-size match state

This is a deliberately narrow first slice of `hider-seeker-complete-design.md`'s
own 12-step implementation roadmap (section 13) — just steps 1 and 2:
freezing the core rules as enums/constants, and implementing the
authoritative fixed-size state block with explicit, versioned
serialization. No tick engine, no HTTP API, no NPC AI yet — those are
later, separate jobs.

See `hider-seeker-complete-design.md` (repo root) for the full system
this is one slice of, and `designer-content-tooling-design.md` for how
content authoring will later sit on top of it. This job does not need to
read either in full; the concrete shape below is self-contained.

## Where this code lives

New files under a new `src/hider_seeker/` directory in this repo
(`ascii-game-backend`) — this is a second, separate game deployed
alongside the existing Asciigame code in the same backend/server
("monolith, both games run on the same servers"), not a change to any
existing Asciigame file. Mirror the internal layout
`hider-seeker-complete-design.md` section 2.3 suggests, just nested
under `src/hider_seeker/` instead of a top-level `/engine-core`, to match
this repo's own existing convention (every subsystem lives under
`src/<name>/`, e.g. `src/model/`, `src/activator/`).

```text
src/hider_seeker/
  domain/
    state.hpp            # the structs below
    serialization.hpp     # save()/load() declarations
    serialization.cpp     # save()/load() implementation
```

## What to build

### 1. Freeze the core rules as real enums/constants

In `src/hider_seeker/domain/state.hpp`, define these as `constexpr` (not
magic numbers scattered through the code), matching the design doc's own
hard capacity limits exactly:

```cpp
namespace hider_seeker {

constexpr uint8_t MAX_HIDERS = 8;
constexpr uint8_t MAX_SERVERS = 16;
constexpr uint8_t MAX_GOALS = 16;
constexpr uint8_t MAX_REQUIREMENTS_PER_GOAL = 16;
constexpr uint8_t MAX_EVENTS = 64;
constexpr uint8_t HISTORY_TICKS = 16;
constexpr uint8_t MAX_INTENTS_PER_TICK = 64;
constexpr uint8_t MAX_SEEKER_TARGETS = 16;
constexpr uint8_t MAX_HIDER_PLAN_STEPS = 16;

}  // namespace hider_seeker
```

Also define, in the same file, the enums the state below references:

- `RewardType` — `REWARD_NONE`, `REWARD` (design doc section 3.4).
- `RequirementType` — `REQ_RESOURCE_MIN`, `REQ_RESOURCE_SPEND`,
  `REQ_ACTIVE_SERVER_COUNT`, `REQ_SERVER_METRIC_THRESHOLD`,
  `REQ_ACTION_COUNT`, `REQ_CONFIGURED_SERVER`, `REQ_CUSTOM_RULE` (section
  3.4).
- `MatchPhase` — at minimum `PHASE_LOBBY`, `PHASE_ACTIVE`,
  `PHASE_COMPLETE` (the design doc references `match.phase` and
  `"ACTIVE"` in its own example API response, section 5.2, but doesn't
  enumerate every phase explicitly — use your judgement for a minimal,
  real set that supports lobby → active → complete, and note any
  assumption you make in your own report).

A single `RESOURCE_TYPE_COUNT`-sized resource enum is NOT in scope for
this slice — `HiderState::resources` below can use a placeholder
constant (e.g. `constexpr uint8_t RESOURCE_TYPE_COUNT = 8;`) since the
real resource catalog is designer-content-tooling-design.md's own,
separate, later concern (that document's section 4).

### 2. The fixed-size state structs

In the same file, implement these structs exactly as sketched in
`hider-seeker-complete-design.md` section 3.1 (reproduced here so this
job doesn't need to cross-reference that file):

```cpp
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

struct Requirement {
    uint8_t type;   // RequirementType
    uint8_t subject_id;
    uint16_t amount;
    uint16_t threshold;
};

struct GoalState {
    uint8_t active;
    uint8_t goal_id;
    uint8_t status;
    uint8_t reward_type;   // RewardType
    uint16_t reward_amount;
    uint8_t requirement_count;   // <= MAX_REQUIREMENTS_PER_GOAL
    Requirement requirements[MAX_REQUIREMENTS_PER_GOAL];
    uint8_t completion_action;
    uint16_t deadline_tick;
};

struct Event {
    // Minimal for this slice: enough to exist as a fixed MAX_EVENTS
    // ring buffer entry. Real event content/audience-visibility fields
    // are a later job's concern (tick engine + projection).
    uint8_t type;
    uint32_t tick;
};

struct MatchState {
    uint32_t schema_version;
    uint32_t ruleset_version;
    uint64_t revision;
    uint32_t tick;
    uint64_t rng_state;
    uint8_t phase;   // MatchPhase
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

Requirements this state must satisfy (design doc section 3.1's own
rules, restated as acceptance criteria):

- No pointers, strings, `std::vector`, or other owning/growing
  containers anywhere inside `MatchState` or anything it contains —
  fixed arrays and PODs only.
- `MatchState` must be a plain, fixed-size, no-dynamic-allocation type —
  a single `MatchState` instance's size must be knowable at compile time
  (a `static_assert` on `sizeof(MatchState)` being some fixed, sane
  upper bound is a reasonable real check to add, though the exact bound
  is your own judgement call, not specified here).

### 3. Explicit, versioned serialization

In `src/hider_seeker/domain/serialization.hpp`/`.cpp`, implement:

```cpp
namespace hider_seeker {

// Serializes `state` into `out` in an explicit, versioned binary format
// -- never a raw memcpy of MatchState's own in-memory layout (the
// design doc's own rule: "do not persist raw compiler memory layout").
// `out` grows as needed; this is the one place in this slice where a
// dynamically-sized container (the serialization buffer itself, not
// game state) is expected and fine.
void saveMatchState(const MatchState& state, std::vector<uint8_t>& out);

// The inverse of saveMatchState. Returns false (leaving `state`
// unspecified) if `in` isn't a recognized, schema_version-compatible
// encoding -- never partially populates `state` on failure.
bool loadMatchState(const std::vector<uint8_t>& in, MatchState& state);

}  // namespace hider_seeker
```

The exact wire format (field order, varint vs. fixed-width, etc.) is
your own implementation decision — the acceptance criterion is that
`saveMatchState` followed by `loadMatchState` round-trips a real
`MatchState` value exactly, and that the format is versioned via
`schema_version` (so a future format change has somewhere real to key
off of), not simply `reinterpret_cast<uint8_t*>(&state)`.

## Requirements

- **HIDER-SEEKER-001**: `src/hider_seeker/domain/state.hpp` defines the
  frozen rule constants, the `RewardType`/`RequirementType`/`MatchPhase`
  enums, and the `MetricFrame`/`ServerState`/`HiderState`/`SeekerState`/
  `Requirement`/`GoalState`/`Event`/`MatchState` structs exactly as
  specified above, with no pointers, strings, or growing containers
  anywhere in `MatchState`.
  - Acceptance criteria:
    - The file compiles as part of this repo's existing CMake build.
    - `MatchState` and every struct it contains use only fixed-size
      arrays, PODs, and the enums/constants defined in the same file.
- **HIDER-SEEKER-002**: `src/hider_seeker/domain/serialization.{hpp,cpp}`
  implements `saveMatchState`/`loadMatchState` with the round-trip and
  versioning behavior described above.
  - Acceptance criteria:
    - A real test constructs a `MatchState` with genuinely varied field
      values (not all zeros/defaults), serializes it, deserializes the
      result, and asserts the two states are equal field-by-field.
    - A real test asserts `loadMatchState` returns `false` (not a crash,
      not a partially-populated `state`) for a buffer that isn't a valid
      encoding (e.g. an empty buffer, or one with a corrupted/unknown
      `schema_version`).

## Out of scope for this job

- The tick engine, intent validation, action resolution, probe
  resolution, or anything else in `hider-seeker-complete-design.md`
  section 3.2 onward.
- The HTTP API (section 5).
- Seeker/hider NPC policy code, training, or ONNX inference (sections 6,
  7, 9) — explicitly optional per the design doc itself, and out of
  scope for this first slice regardless.
- The designer content-authoring tooling (the separate
  `designer-content-tooling-design.md`).
- WASM build wiring for this code (the design doc's own note that "the
  same domain source may be compiled for server and WASM" is a real,
  later requirement — this slice only needs the code to be valid,
  portable C++ that COULD later compile under Emscripten, not an actual
  WASM build target yet).
- Any change to existing Asciigame files under `src/` outside the new
  `src/hider_seeker/` directory.
