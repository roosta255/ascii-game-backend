# Traits configuration (what changed for the field system)

This is a one-pager on `Trait.enum`, the data-driven trait system, and
specifically what the field system's two commits (`feat: fields`,
`feat: fields remaining`) added to it. For the field system itself, see
[claude-fields.md](claude-fields.md).

## How `Trait.enum` works (existing system, unchanged)

`src/traits/Trait.enum` is an X-macro table, one row per trait:

```cpp
TRAIT_DECL(name_, { TRAIT_TYPE_..., TRAIT_TYPE_... })
    TRAIT_MODIFIER_DECL(is_global_, ({ {required}, {restricted}, {set}, {clear} }))
```

- `TraitEnum.hpp` expands every row into a `TRAIT_NAME` value in the `TraitEnum` enum.
- `TraitFlyweight.cpp` expands every row into metadata (its `TraitType` tags, plus
  any `TRAIT_MODIFIER_DECL` rows attached to it).
- A `TraitModifier` fires when the character's current trait set contains all of
  `required` and none of `restricted`; firing sets/clears bits. `TraitModifier::computeTraits`
  runs this to a fixed point (bounded, `TRAIT_COUNT * 4` iterations max).
- Traits attach to a character from three places: `RoleFlyweight::traitsSourced` (what
  the role innately grants), `Character::traitsAfflicted` (bites, statuses, environmental
  afflictions), and now `RoomFlyweight::roomSourceAttributes` (below).

`TraitType` tags (`TRAIT_TYPE_SOURCE`, `_AFFLICTION`, `_DERIVED`, `_CHARACTER`,
`_DUNGEON`, `_DOOR`, `_ITEM`, ...) are just classification bits used for readability
and by some downstream code (e.g. door-blocking checks); they don't affect how the
modifier engine evaluates required/restricted/set/clear.

## What the field system added: two room-scoped source traits

```cpp
// --- ENVIRONMENTAL FIELD SOURCE ATTRIBUTES (room-scoped, see RoomFlyweight::roomSourceAttributes) ---
TRAIT_DECL(OXYGEN_VENT,         ({ TRAIT_TYPE_SOURCE, TRAIT_TYPE_DUNGEON }))
TRAIT_DECL(HEAT_SOURCE,         ({ TRAIT_TYPE_SOURCE, TRAIT_TYPE_DUNGEON }))
```

Both use `TRAIT_TYPE_DUNGEON`, the same tag every other room/door-scoped source
trait already uses (`IS_DOOR_BLOCKING`, `LOCK_OPEN`, etc.) — `TRAIT_TYPE_ROOM` exists
in `TraitType.hpp` but has never actually been used by any trait row, so `_DUNGEON`
is the real convention to follow, not `_ROOM`.

These have no modifiers — they're pure flags a room type either has or doesn't.

### Where they attach

`RoomFlyweight::roomSourceAttributes` (a `TraitBits`) was already a field on every
room type before this work, but every `Room.enum` row passed `{}` — an unused slot.
Two rows now populate it:

```cpp
ROOM_DECL(LIGHTNING_ROD,   4, 1, ..., {TRAIT_HEAT_SOURCE})
ROOM_DECL(POWER_GENERATOR, 4, 5, ..., {TRAIT_OXYGEN_VENT})
```

### Where they're read

The field system's source providers query `RoomFlyweight::getFlyweights()` for the
room's `type` and check the bit:

```cpp
RoomFlyweight::getFlyweights().accessConst(room.type, [&](const RoomFlyweight& flyweight) {
    if (flyweight.roomSourceAttributes[TRAIT_OXYGEN_VENT].orElse(false)) { ... }
});
```

`OxygenSourceProvider` checks `TRAIT_OXYGEN_VENT` (a room in a `POWER_GENERATOR`
gets +40 oxygen). `HeatSourceProvider` checks `TRAIT_HEAT_SOURCE` (a room in a
`LIGHTNING_ROD` radiates heat). See
[fields-configuration.md](fields-configuration.md) for the numbers.

## A pre-existing trait that became meaningful: `TRAIT_ENFLAMED`

`TRAIT_ENFLAMED` (`TRAIT_TYPE_AFFLICTION, TRAIT_TYPE_CHARACTER, TRAIT_TYPE_STATE`)
already existed before either fields commit, set by nothing, read by nothing — a
dormant "this character is on fire" flag. The field system is the first thing to
actually read it: `OxygenSourceProvider`, `HeatSourceProvider`, and
`SmokeSourceProvider` all check `character.traitsAfflicted[TRAIT_ENFLAMED]` for every
used character in a room, treating an enflamed character as an oxygen sink and a
heat/smoke source. No `Trait.enum` change was needed for this — only the field
providers reading an existing bit for the first time.

## What did *not* change

`Role.enum` has no substantive changes in either fields commit (confirmed via
whitespace-insensitive diff against the commit before `feat: fields`) — no new
roles, no new role-granted traits. All of the trait work for this feature is the
two room-scoped source traits above.
