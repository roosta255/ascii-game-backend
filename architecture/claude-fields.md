# Field System Implementation Plan (grounded revision)

This is a rewrite of [chatgpt-fields.md](chatgpt-fields.md) after checking every
structural claim in it against the actual engine. The original design is
directionally sound — this version keeps its goals, swaps every invented
primitive for the equivalent one that already exists in `src/`, and flags the
two places where the original's assumptions don't hold once you look at how
`Match` actually gets constructed per request.

## Purpose (unchanged)

A generic dungeon Field system for derived environmental state: Heat, Oxygen,
Smoke, and later Poison Gas / Light / Sound. Puzzle mechanics, not simulation.
Deterministic, event-driven recomputation, minimal CPU cost, data-driven
configuration, reusable resolver algorithms.

---

## 1. What already exists that this should reuse

| Design element | Reuse this instead of inventing it |
|---|---|
| Room cap | [`DUNGEON_ROOM_COUNT`](../src/layout/DUNGEON_ROOM_COUNT.hpp) = 64, already the real ceiling (`Array<Room, DUNGEON_ROOM_COUNT> rooms` in [Dungeon.hpp:36](../src/model/Dungeon.hpp)) |
| Room-indexed bitset | [`Bitstick<N>`](../src/data/Bitstick.hpp) — already instantiated as `TraitBits = Bitstick<TRAIT_COUNT>` ([TraitBits.hpp](../src/traits/TraitBits.hpp)) and used room-scoped in `Bitstick<DUNGEON_ROOM_COUNT>` inside `PathfindingCounter` |
| Data-driven enum + metadata | The X-macro `.enum` pattern used by every other subsystem: [Action.enum](../src/activator/Action.enum) → [ActionEnum.hpp](../src/activator/ActionEnum.hpp) → [ActionFlyweight.cpp](../src/activator/ActionFlyweight.cpp), and [Trait.enum](../src/traits/Trait.enum) → [TraitFlyweight.cpp](../src/traits/TraitFlyweight.cpp) |
| "Recompute derived state, cache until invalidated" | `MatchController::traitsComputed` + `updateTraits()` ([MatchController.cpp:1161](../src/controllers/MatchController.cpp)) — this is a much closer precedent than any cache, see §3 |
| Fixed-point relaxation over a bitset | `TraitModifier::computeTraits` ([TraitModifier.cpp:82](../src/traits/TraitModifier.cpp)) — bounded `while (changed)` loop over all flyweights, capped at `TRAIT_COUNT * 4` iterations |
| BFS over rooms | `MatchController::floodFillRoom` / `floodFillRoomBits` ([MatchController.cpp:432](../src/controllers/MatchController.cpp)) |
| Per-room source-attribute slot, already wired but unused | `RoomFlyweight::roomSourceAttributes` (`TraitBits`) — every [Room.enum](../src/room/Room.enum) row currently passes `{}` |
| Boolean environmental-affliction plumbing | `Character::traitsAfflicted` (`TraitBits`) + `ActivatorAlterTraitAffliction` — `TRAIT_ENFLAMED` already exists ([Trait.enum:147](../src/traits/Trait.enum)) as an unused state trait, a ready-made "this character is a heat/smoke source" flag |
| Turn-boundary hook | `Turner::advanceTitanTurnState` / `advanceBuilderTurnState` / `runNpcTurn` ([Turner.cpp](../src/model/Turner.cpp)) — every character's `startTurn`/`endTurn` already funnels through exactly these three call sites |
| Empty hook for per-character consequences | `Character::endTurn(Match&)` ([Character.cpp:143](../src/model/Character.cpp)) is currently a literal no-op |

None of this is a stretch — it's what the codebase already does for the
closest analogous problem (derived character state from trait modifiers).

---

## 2. What genuinely doesn't exist yet

- **No environmental simulation of any kind.** `DAMAGE_TYPE_FIRE`/`DAMAGE_TYPE_HEAT`
  ([DamageType.enum](../src/damage/DamageType.enum)) are one-shot combat flavor —
  `ActivatorDamageFire::activate` is just `target.damage += 1;`
  ([ActivatorDamageFire.cpp](../src/damage/ActivatorDamageFire.cpp)). No Smoke,
  Oxygen, or Gas mechanic exists at all.
- **No numeric per-character timer.** `grep -rn "Timer\b" src/` is empty.
  Character status today is 100% boolean (`TraitBits traitsAfflicted`). A
  `hypoxiaTimer`-style counter is a new field on `Character`, not a
  reinterpretation of something that exists.
- **No dirty-bit invalidation bus.** Nothing marks fine-grained sub-state
  dirty today; `traitsComputed` is unconditionally rebuilt for every used
  character in the `MatchController` constructor and then explicitly patched
  per-character by whichever activator just touched that character. That's
  "recompute on mutation," not "check a dirty flag before recomputing" — see
  next section for why that distinction matters here.
- **Raw `1ULL << x` bitset math does not appear anywhere in `src/`.** The one
  place a raw int bitmask over rooms exists — `floodFillRoomBits`
  ([MatchController.cpp:432](../src/controllers/MatchController.cpp),
  `int startRoomBits`) — only has 31 usable bits for a 64-room dungeon. This
  is a known latent gap in the existing code, not a pattern to copy.

---

## 3. The load-bearing fact the original doc missed: request lifecycle

`Match` is not a long-lived in-memory world. Every API call
([ApiController.cpp](../src/controllers/ApiController.cpp)) loads the match's
JSON blob from disk via `FileStore::load` ([FileStore.cpp:78](../src/store/FileStore.cpp)),
deserializes it, constructs a **stack-local** `MatchController controller(match, codeset)`
for the duration of that one request, and `FileStore::save`s the updated JSON
back out with a bumped `version`. `MatchController`'s constructor eagerly
recomputes `traitsComputed` for every used character on every single request:

```cpp
// MatchController.cpp:31
MatchController::MatchController(Match& match, Codeset& codeset): match(match), codeset(codeset) {
    match.accessUsedCharacters([&](const Character& character) {
        traitsComputed.set(character.characterId, TraitModifier::computeCharacterTraits(character).orElse(TraitModifier::TraitComputation{}));
    });
}
```

The comment on the member is explicit: `// characterId -> computed traits
(always fresh, never persisted)` ([MatchController.hpp:57](../src/controllers/MatchController.hpp)).

This means a `dirtyRooms` bitset only saves work **within** one HTTP request
(across multiple activations in the same `activate()` call chain). It cannot
save work **across** requests unless the field *values* — and the dirty bits
— are added to `Dungeon` itself and round-tripped through the save/load
cycle. That's a real design fork the original doc doesn't surface:

- **Option A (matches `traitsComputed` precedent exactly):** don't persist
  field values or dirty bits at all. Store `Array<int16_t, DUNGEON_ROOM_COUNT>`
  per field on a `FieldController` owned by `MatchController`, and recompute
  all fields unconditionally in the constructor, the same way `traitsComputed`
  is rebuilt every request. With ≤64 rooms and a handful of fields, this is
  cheap, requires zero new persisted state, and needs zero dirty-bit
  bookkeeping at all — the "dirty tracking" half of the original doc may
  simply not earn its complexity at this scale.
- **Option B (what the original doc assumes):** persist field values (and
  optionally dirty bits) on `Dungeon`, so they survive across requests and
  only recompute when something changed. This is the right call only if full
  recompute of all fields turns out to be measurably expensive — which, for
  ≤64 rooms and resolvers no fancier than BFS/relaxation, it almost certainly
  isn't at first.

**Recommendation: build Option A first.** It's strictly less code, has a
direct precedent already proven in production (`traitsComputed`), and can be
upgraded to Option B later if profiling says otherwise. Don't build the dirty
bitset until you've measured that unconditional recompute is a problem.

---

## 4. Field storage

```cpp
// FieldFlyweight-adjacent runtime state, owned by MatchController (Option A)
struct FieldState {
    std::array<int16_t, DUNGEON_ROOM_COUNT> values{};
    Bitstick<DUNGEON_ROOM_COUNT> flags; // e.g. hypoxiaRooms, smokeRooms — derived, recomputed alongside values
};
```

Use `DUNGEON_ROOM_COUNT` (already `#include`d everywhere room arrays live —
`Dungeon.hpp`, `Room.hpp`, `MatchController.hpp`), not a hardcoded `64`. Use
`Bitstick<DUNGEON_ROOM_COUNT>` for any per-room flag set, not `uint64_t` —
`Bitstick` already gives you `|`, `&`, `-`, `contains`, `shares`,
`setIndexOn/Off`, and `operator[]` returning `Maybe<bool>`, all of which the
rest of the codebase already uses for exactly this shape of data
(`TraitBits`, `Bitstick<DUNGEON_ROOM_COUNT>` in `PathfindingCounter`).

If Option B is later warranted, `dirtyRooms` also becomes a
`Bitstick<DUNGEON_ROOM_COUNT>`, living on `Dungeon` next to `rooms`.

---

## 5. Data-driven `Field.enum` — following the existing X-macro convention exactly

The codebase already generates enums + flyweights from a `.enum` file for
every comparable subsystem (`Action.enum`, `Trait.enum`, `DamageType.enum`,
`Room.enum`). `Field.enum` should be the same shape.

```cpp
// src/field/Field.enum
// FIELD_DECL(name_, resolver_, sourceProvider_, invalidationMask_, minValue_, maxValue_)
FIELD_DECL(HEAT,   DISTANCE_FIELD,    HeatSourceProvider,   (CHARACTER | OBJECT | ROOM), -100, 100)
FIELD_DECL(OXYGEN, ROOM_ACCUMULATION, OxygenSourceProvider, (CHARACTER | ROOM | DOOR),      0, 100)
FIELD_DECL(SMOKE,  RELAXATION,        SmokeSourceProvider,  (CHARACTER | OBJECT),            0, 255)
```

```cpp
// src/field/FieldEnum.hpp — mirrors ActionEnum.hpp exactly
#pragma once
#include <string>

#define FIELD_DECL(name_, resolver_, provider_, mask_, min_, max_) FIELD_##name_,
enum FieldEnum
{
#include "Field.enum"
FIELD_COUNT
};
#undef FIELD_DECL

const char* field_to_text(const FieldEnum&);
```

```cpp
// src/field/FieldFlyweight.hpp — mirrors ActionFlyweight.hpp
#pragma once
#include "Array.hpp"
#include "FieldEnum.hpp"
#include "Pointer.hpp"

class iFieldResolver;
class iFieldSourceProvider;

struct FieldFlyweight {
    std::string name;
    Pointer<const iFieldResolver> resolver;
    Pointer<const iFieldSourceProvider> sourceProvider;
    int invalidationMask;
    int16_t minValue, maxValue;

    static const Array<FieldFlyweight, FIELD_COUNT>& getFlyweights();
};
```

```cpp
// src/field/FieldFlyweight.cpp — mirrors ActionFlyweight.cpp's generator block
const Array<FieldFlyweight, FIELD_COUNT>& FieldFlyweight::getFlyweights() {
    static auto flyweights = [](){
        Array<FieldFlyweight, FIELD_COUNT> flyweights;

        #define FIELD_DECL(name_, resolver_, provider_, mask_, min_, max_) \
            static resolver_##FieldResolver GLOBAL_##name_##_resolver; \
            static provider_ GLOBAL_##name_##_provider; \
            flyweights.getPointer(FIELD_##name_).access([&](FieldFlyweight& f){ \
                f.name = #name_; \
                f.resolver = GLOBAL_##name_##_resolver; \
                f.sourceProvider = GLOBAL_##name_##_provider; \
                f.invalidationMask = mask_; \
                f.minValue = min_; f.maxValue = max_; \
            });
        #include "Field.enum"
        #undef FIELD_DECL

        return flyweights;
    }();
    return flyweights;
}
```

This is the exact `ACTION_DECL`/`GLOBAL_##name_##activation_intf_` idiom from
[ActionFlyweight.cpp:24-42](../src/activator/ActionFlyweight.cpp), just
renamed. `resolver_##FieldResolver` selects one of a small fixed set of
resolver classes (§7) via string-pasting on the resolver name, the same way
`DAMAGE_TYPE_DECL` selects `ActivatorDamageFire`/`ActivatorDamageCrush`/etc.
per damage type.

If specific fields need per-flag thresholds (e.g. "HYPOXIA fires below 30,
OXYGENATED fires above 70"), follow `Trait.enum`'s nested-declaration
pattern — `TRAIT_MODIFIER_DECL` rows attach to the preceding `TRAIT_DECL` via
a `lastField` tracker variable exactly like `lastTrait` in
[TraitFlyweight.cpp:21-36](../src/traits/TraitFlyweight.cpp). A
`FIELD_FLAG_DECL(flagName_, thresholdOp_, thresholdValue_)` sub-row under each
`FIELD_DECL` would work the same way.

---

## 6. Source collection

Sources should be *discovered* from `Dungeon`/`Character` state each
recompute, not duplicated into the field itself — this matches how
`TraitModifier::computeCharacterTraits` re-derives everything from
`character.traitsAfflicted` and `RoleFlyweight::traitsSourced` on every call
rather than storing a separate cache of "why is this trait set."

Concretely, a source provider for Heat would:

- Iterate `match.dungeon.characters` (via `Match::accessAllCharacters`,
  [Match.hpp:36](../src/model/Match.hpp)) and check
  `character.traitsAfflicted[TRAIT_ENFLAMED]` — an existing trait, currently
  set by nothing, that becomes real the moment this system reads it.
- Iterate `match.dungeon.rooms` and check
  `RoomFlyweight::roomSourceAttributes` for a not-yet-declared
  `TRAIT_HEAT_SOURCE`/`TRAIT_LAVA` bit — the slot already exists on every
  `Room.enum` row ([Room.enum](../src/room/Room.enum), all currently `{}`),
  it just needs a non-empty declaration and a room type that uses it.
- Iterate inventory/world objects for heat-emitting items once those exist
  (there is no torch/lava object model yet — this part is genuinely new).

This is also where the invalidation mask from `Field.enum`
(`CHARACTER | OBJECT | ROOM`) earns its keep *if* you go with Option B —
activators that touch a character's `traitsAfflicted`, a room's
`roomSourceAttributes`, or spawn/despawn an object would OR their category
into a dirty mask, the same way `ActivatorAlterTraitAffliction` calls
`controller.updateTraits(subject)` immediately after mutating
`traitsAfflicted` ([ActivatorAlterTraitAffliction.cpp:25](../src/activator/ActivatorAlterTraitAffliction.cpp)).
Under Option A this mask is unused — everything just recomputes.

---

## 7. Resolvers — model these on code that already exists

| Resolver | Existing analog | Notes |
|---|---|---|
| `ROOM_ACCUMULATION` (Oxygen) | Plain per-room sum, no analog needed | `base + vents - consumption`, one pass over `Dungeon::rooms` |
| `DISTANCE_FIELD` (Heat) | `MatchController::floodFillRoom` BFS ([MatchController.cpp:432](../src/controllers/MatchController.cpp)) | Same traversal shape — walk `Dungeon`'s room adjacency (`Room::anterior/posterior/above/below`, `Wall`), decrementing value with distance from each source room instead of the current "reachable in N moves" semantics |
| `RELAXATION` (Smoke) | `TraitModifier::computeTraits` fixed-point loop ([TraitModifier.cpp:82-128](../src/traits/TraitModifier.cpp)) | Same shape: `while (changed && iterations++ < MAX_ITERATIONS)`, iterate all rooms, push a bounded amount toward neighbor equilibrium, track `changed`. The existing code's `MAX_ITERATIONS = TRAIT_COUNT * 4` safety cap is the right idiom to copy (`DUNGEON_ROOM_COUNT * 4` here) so a bad configuration can't hang a request instead of oscillating forever |
| `ROOM_EQUALIZATION` (airlock open) | No existing analog | Straightforward: average two `Room`'s values on the door-open event; simplest resolver to add last |

Room adjacency for `DISTANCE_FIELD`/`RELAXATION` should reuse `Dungeon`'s
existing neighbor accessors — `accessCeilingNeighbor`, `accessFloorNeighbor`,
`accessWallNeighbor` ([Dungeon.hpp:59-66](../src/model/Dungeon.hpp)) — rather
than a new adjacency structure.

---

## 8. Turn lifecycle integration

Hook a single `FieldController::recompute(match)` call into the three
existing turn-boundary functions in [Turner.cpp](../src/model/Turner.cpp):

- `advanceTitanTurnState` (titan's `endTurnProcess` lambda, line 139)
- `advanceBuilderTurnState` (builders' `endTurnProcess` lambda, line 166)
- `runNpcTurn` (line 192, after `tickConducts()`, before the
  `character.endTurn(match)` loop)

Then `Character::endTurn(Match&)` — currently:

```cpp
// Character.cpp:143
void Character::endTurn(Match& match)
{
    // Currently no end-of-turn cleanup needed
}
```

— becomes the per-character consumer of derived room flags, e.g.:

```cpp
void Character::endTurn(Match& match) {
    if (match.dungeon.getRoom(location.roomId, /*error*/).accessConst([&](const Room&){ ... }))
        if (fieldController.hypoxiaRooms[location.roomId].orElse(false))
            hypoxiaTimer++; // new field — see §2, does not exist today
}
```

This matches the doc's own principle ("environmental duration belongs to
characters, not fields") and slots into the one place in the turn loop that's
already an empty hook waiting for exactly this kind of logic.

---

## 9. Where `FieldController` lives

Given §3, the natural home (Option A) is a member of `MatchController`
alongside `traitsComputed`, constructed the same way:

```cpp
// MatchController.hpp, alongside the existing:
// Map<int, TraitModifier::TraitComputation> traitsComputed;
FieldController fields;

// MatchController.cpp constructor, alongside the existing traitsComputed rebuild:
MatchController::MatchController(Match& match, Codeset& codeset): match(match), codeset(codeset), fields(match.dungeon) {
    ...
}
```

`FieldController::get(FieldEnum, int roomId)` becomes a thin read of the
already-recomputed `FieldState`, exactly mirroring
`MatchController::getTraitsComputed(characterId)`
([MatchController.cpp:1165](../src/controllers/MatchController.cpp)).

---

## 10. Design rules to keep from the original doc

These held up under scrutiny and should stay as-is:

- No fluid dynamics / velocity vectors / pressure simulation / per-tile gas.
- Fields are cached answers to "what is this room's state," not a simulation.
- Characters query derived flags, not raw scalars.
- Add a field only when it creates a new puzzle mechanic, not for realism.

---

## 11. Suggested implementation order

1. `Field.enum` + `FieldEnum.hpp` + `FieldFlyweight.hpp/cpp`, one field only
   (`OXYGEN`, since `ROOM_ACCUMULATION` needs no adjacency/BFS/relaxation).
2. `FieldController` as a `MatchController` member (Option A: full recompute
   in the constructor, no dirty bits yet).
3. Wire `Turner`'s three turn-boundary sites to call
   `FieldController::recompute`.
4. Add `TRAIT_HEAT_SOURCE`-equivalent declarations to `RoomFlyweight` /
   `Room.enum` and wire `OxygenSourceProvider` to read them plus
   `TRAIT_ENFLAMED`-consumption from characters.
5. Add the `hypoxiaTimer`-equivalent field to `Character`, consumed in
   `Character::endTurn`.
6. Add `HEAT` with `DISTANCE_FIELD`, reusing `Dungeon`'s neighbor accessors
   for traversal.
7. Add `SMOKE` with `RELAXATION`, modeled on `TraitModifier::computeTraits`'s
   bounded fixed-point loop.
8. Only if profiling shows full-recompute cost matters: promote field values
   (and a `Bitstick<DUNGEON_ROOM_COUNT> dirtyRooms`) onto `Dungeon` for
   cross-request persistence (Option B), gated by the `Field.enum`
   invalidation mask that activators already have a natural call site to set
   (next to their existing `controller.updateTraits(...)` calls).
9. `ROOM_EQUALIZATION` for airlock-style room merges, last — no existing code
   shape to lean on, lowest risk to defer.
