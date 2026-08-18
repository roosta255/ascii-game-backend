# Fields configuration (what `Field.enum` is and how it works)

This is a one-pager on the `Field.enum` data-driven config table itself — the
concrete rows that exist today and how each one resolves to a number. For the
full design rationale (why Option A over Option B, request lifecycle, etc.) see
[claude-fields.md](claude-fields.md).

## The table

`src/field/Field.enum`, one row per field:

```cpp
// FIELD_DECL(name_, resolver_, sourceProvider_, invalidationMask_, minValue_, maxValue_)
FIELD_DECL(OXYGEN, ROOM_ACCUMULATION, OxygenSourceProvider, 0, 0, 100)
FIELD_DECL(HEAT,   DISTANCE_FIELD,    HeatSourceProvider,   0, -100, 100)
FIELD_DECL(SMOKE,  RELAXATION,        SmokeSourceProvider,  0, 0, 255)
```

Same X-macro shape as `Trait.enum`/`Room.enum`: `FieldEnum.hpp` expands it into
`FIELD_OXYGEN`/`FIELD_HEAT`/`FIELD_SMOKE`/`FIELD_COUNT`; `FieldFlyweight.cpp`
expands it into a static `Array<FieldFlyweight, FIELD_COUNT>`, one entry per row,
each holding a `resolver` and `sourceProvider` (both name-pasted from column 2/3,
e.g. `DISTANCE_FIELDFieldResolver` + `HeatSourceProvider`), plus `minValue`/`maxValue`
for clamping. `invalidationMask` is currently unused by every row (all `0`) — it
only matters if the system is ever promoted to Option B (persisted, dirty-bit-gated
fields); today everything just recomputes unconditionally every request.

## The two interfaces every field plugs into

- **`iFieldSourceProvider::getRoomBaseValue(match, roomId)`** — returns one room's
  *raw* contribution, read fresh from `Match`/`Dungeon`/`Character` state. No
  caching, no stored "why" — same philosophy as `TraitModifier` re-deriving
  everything from source state on every call.
- **`iFieldResolver::resolve(match, source, flyweight, state)`** — turns all 64
  rooms' raw contributions into the field's final per-room `FieldState::values`
  (an `int16_t[DUNGEON_ROOM_COUNT]`, clamped to `[minValue, maxValue]`).

`FieldController::recompute(match)` runs every row's resolver+provider pair and
is called from `MatchController`'s constructor and after every turn advance —
see claude-fields.md §3/§9 for why that's the right lifecycle.

## The three fields, concretely

### OXYGEN — `ROOM_ACCUMULATION`

Per-room, no adjacency. `OxygenSourceProvider`:

```
value = 60 (ambient)
      + 40 if the room's type has TRAIT_OXYGEN_VENT   (e.g. POWER_GENERATOR)
      − 20 per ENFLAMED character standing in that room
```

`ROOM_ACCUMULATIONFieldResolver` just clamps that to `[0, 100]` per room — no
cross-room interaction at all, which is why this was the first field built (no
BFS/relaxation needed).

### HEAT — `DISTANCE_FIELD`

`HeatSourceProvider` marks a room as a heat source (base value `100`) if either
its room type has `TRAIT_HEAT_SOURCE` (e.g. `LIGHTNING_ROD`) or an ENFLAMED
character is standing in it. `DISTANCE_FIELDFieldResolver` then multi-source
BFSes outward from every source room using `Dungeon`'s existing neighbor
accessors (`accessCeilingNeighbor`, `accessFloorNeighbor`, `accessWallNeighbor`
across all 4 cardinals), subtracting 25 per hop, clamped to `[-100, 100]`. Rooms
unreachable from any source stay at `0`.

### SMOKE — `RELAXATION`

`SmokeSourceProvider` returns `80` per ENFLAMED character standing in a room (no
room-type source for smoke today — only characters on fire produce it).
`RELAXATIONFieldResolver` runs a bounded fixed-point loop (capped at
`DUNGEON_ROOM_COUNT * 4` iterations, the same safety-cap idiom as
`TraitModifier::computeTraits`): each pass, every room's value moves toward the
average of itself and its connected neighbors, but never below its own source
value (so a room that's actively producing smoke can't be diluted below its own
output). Rooms with no connections and no source stay at `0`.

## Reading a field's value

`MatchController::getFieldValue(FieldEnum, roomId)` — a thin, always-fresh read,
mirroring `getTraitsComputed(characterId)`. There's also
`FieldController::isRoomHypoxic(match, roomId)`, a static, no-recompute-needed
check used by `Character::endTurn` to drive `Character::hypoxiaTimer` (a room is
hypoxic when its OXYGEN value would be `< 30`) — see claude-fields.md §8 for why
this bypasses the full `FieldController` (pathfinding needs cheap speculative
turn-advances without paying for a recompute).

## Adding a new field

1. Write an `iFieldSourceProvider` (per-room raw value) — reuse `ROOM_ACCUMULATION`,
   `DISTANCE_FIELD`, or `RELAXATION` as the resolver if the new field's shape
   matches one of the three above; only write a new resolver class for a genuinely
   new propagation shape.
2. Add one line to `Field.enum`.
3. `#include` the new provider/resolver headers in `FieldFlyweight.cpp`.
4. Add the new `.cpp` files to `CMakeLists.txt` (`core_backend` target — file list
   is explicit, not globbed).
