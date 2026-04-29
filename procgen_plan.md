# Procedural Dungeon Generation — Implementation Plan

## Architecture Summary

This system replaces the generate→solve→fix loop with a **constructive compiler**: small,
pre-validated puzzle fragments ("atoms") are composed in order, each annotated with the
invariants it maintains. Solvability is guaranteed by construction, not by post-hoc search.

Mental model: this is a **compiler from a logical puzzle graph to a spatial dungeon**, not a
pathfinder or a constraint solver.

---

## 0. Codebase Context

Key integration targets identified from the existing code:

| Symbol | Location | Role in new system |
|--------|----------|--------------------|
| `iGenerator` | `src/generator/iGenerator.hpp` | Interface to implement |
| `DungeonMutator` | `src/generator/DungeonMutator.hpp` | Only write API we call |
| `DungeonAuthor` | `src/generator/DungeonAuthor.hpp` | Room allocation in 4D grid |
| `LayoutGrid` | `src/layout/LayoutGrid.hpp` | Coord↔index translation |
| `Dungeon::isBlueOpen` | `src/model/Dungeon.hpp` | Runtime toggle phase |
| `DUNGEON_ROOM_COUNT` | `src/model/Dungeon.hpp` | Room budget (96) |
| `DoorEnum` | `src/door/Door.enum` | Target door types |
| `ITEM_KEY` | `src/inventory/Item.enum` | Key item type |
| `ROLE_TOGGLER_BLUE/ORANGE` | `src/role/Role.enum` | Toggler character types |
| `Generator.enum` | `src/generator/Generator.enum` | Registration point |
| `Wall::adjacent` | `src/room/Wall.hpp` | Room adjacency pointer |

Mutator methods used by the compiler:
- `setupDoorway(roomA, roomB, direction)`
- `setupKeeper(roomA, roomB, direction, keyRoomId)`
- `setupJailer(roomA, roomB, direction)`
- `setupShifter(roomA, roomB, direction)`
- `setupTogglerBlue(roomA, roomB, direction)`
- `setupTogglerSwitch(roomId)` — places toggler character
- `setupExitDoor(roomId, direction)`
- `setDoor(roomId, direction, DoorEnum)`

---

## 1. Data Structures (new files in `src/procgen/`)

### 1.1 `Atom.hpp`

```cpp
// Door semantic used inside an atom definition.
// The compiler maps these → DungeonMutator calls.
enum class AtomDoor {
    DOORWAY,
    KEEPER,       // key provided somewhere in this atom
    JAILER,
    SHIFTER,
    TOGGLER_BLUE, // open when isBlueOpen == true
    TOGGLER_ORANGE,
    EXIT,
};

struct AtomNode {
    int id;       // local index (0..N-1)
    int dx, dy;   // relative grid offset from atom origin
    bool isEntry;
    bool isExit;
};

struct AtomEdge {
    int from, to;
    AtomDoor door;
    int keyNodeId; // for KEEPER/SHIFTER: which node holds the key (-1 if none)
};

// Compile-time invariant annotations.
struct AtomMeta {
    bool requiresBlueOpen;  // entry phase requirement (only meaningful if phaseSensitive)
    bool phaseSensitive;    // does this atom care about entry phase?
    bool flipsPhase;        // does this atom place a toggler switch?
    int keyRequired;        // keys needed in inventory at entry
    int keyProvided;        // keys placed in this atom's rooms
    int keyConsumed;        // keys spent traversing this atom's critical path
    // net key delta = keyProvided - keyConsumed
};

struct Atom {
    const char* name;
    Array<AtomNode, 8> nodes;
    int nodeCount;
    Array<AtomEdge, 12> edges;
    int edgeCount;
    AtomMeta meta;
};
```

### 1.2 `ArchitectState.hpp`

Compile-time constraint tracker — not gameplay state.

```cpp
struct ArchitectState {
    bool isBlueOpen;  // current phase assumption
    int keyBalance;   // keys available to the player at this point in construction

    bool canAccept(const AtomMeta& meta) const {
        if (meta.phaseSensitive && meta.requiresBlueOpen != isBlueOpen) return false;
        if (keyBalance + meta.keyProvided - meta.keyRequired < 0) return false;
        return true;
    }

    void apply(const AtomMeta& meta) {
        keyBalance += meta.keyProvided;
        keyBalance -= meta.keyConsumed;
        if (meta.flipsPhase) isBlueOpen = !isBlueOpen;
    }
};
```

Invariant enforced at every atom placement: `keyBalance >= 0` after `apply()`.

### 1.3 `AtomLibrary.hpp/.cpp`

Static collection of hand-authored atoms. Initially ~8 atoms; can grow.

Atom catalogue (see Section 3 for full definitions):
- `ATOM_KEEPER_LINEAR` — simple key-and-door, phase-neutral
- `ATOM_KEEPER_LOOP` — key behind detour, main path gated
- `ATOM_TOGGLER_CORRIDOR` — blue-open corridor, phase-sensitive
- `ATOM_TOGGLER_ORANGE_CORRIDOR` — orange-open corridor
- `ATOM_PHASE_FLIP` — places toggler switch, flips phase
- `ATOM_SHIFTER_GUARDED` — shifter with guaranteed return path
- `ATOM_JAILER_BYPASS` — jailer with alternate route
- `ATOM_FREE_KEY` — places a free key, no gates

### 1.4 `AtomEmbedder.hpp/.cpp`

Maps atom nodes → real room indices using `DungeonAuthor` + `LayoutGrid`.

```cpp
struct EmbeddedAtom {
    const Atom* atom;
    int origin;          // room index of node[0]
    int roomIds[8];      // roomIds[i] = real room index for atom.nodes[i]
};

class AtomEmbedder {
public:
    // Find a free region, allocate rooms, return EmbeddedAtom.
    // Returns nullopt if no valid placement found.
    Maybe<EmbeddedAtom> embed(
        const Atom& atom,
        int baseX, int baseY,
        const DungeonAuthor& author,
        const LayoutGrid& grid,
        const OccupancyMap& occupied
    );
private:
    Cardinal computeDirection(int dx, int dy);
};
```

`OccupancyMap` is a `Bitstick<DUNGEON_ROOM_COUNT>` tracking allocated rooms.

**Direction convention** (matches `Wall::walls[0..3]` ordering as defined in `Room.hpp`):
- `(dx=+1, dy=0)` → EAST
- `(dx=-1, dy=0)` → WEST
- `(dx=0, dy=-1)` → NORTH
- `(dx=0, dy=+1)` → SOUTH

Verify this against `Cardinal` enum and `Room::walls` indexing before finalizing.

### 1.5 `AtomCompiler.hpp/.cpp`

Translates an `EmbeddedAtom` → `DungeonMutator` calls.

```cpp
class AtomCompiler {
public:
    void compile(const EmbeddedAtom& embedded, DungeonMutator& mutator);
private:
    void compileEdge(const AtomEdge& edge, const EmbeddedAtom& em, DungeonMutator& mutator);
};
```

Per-edge dispatch:
- `DOORWAY` → `mutator.setupDoorway(roomA, roomB, dir)`
- `KEEPER` → `mutator.setupKeeper(roomA, roomB, dir, keyRoom)`
- `JAILER` → `mutator.setupJailer(roomA, roomB, dir)`
- `SHIFTER` → `mutator.setupShifter(roomA, roomB, dir)` + validate return path invariant
- `TOGGLER_BLUE` → `mutator.setupTogglerBlue(roomA, roomB, dir)`
- `TOGGLER_ORANGE` → use orange variant (check DungeonMutator for method name)
- For atoms with `flipsPhase=true`: `mutator.setupTogglerSwitch(switchRoomId)`

### 1.6 `GeneratorAtom.hpp/.cpp`

Implements `iGenerator`. Orchestrates the full pipeline.

```cpp
class GeneratorAtom : public iGenerator {
public:
    bool generate(int seed, Match& match, Codeset& codeset) const override;
private:
    Maybe<EmbeddedAtom> placeAtom(
        const Atom& atom,
        ArchitectState& state,
        OccupancyMap& occupied,
        DungeonAuthor& author,
        LayoutGrid& grid
    );
    const Atom* selectAtom(const ArchitectState& state, int seed, int step) const;
};
```

Generation loop (see Section 2).

---

## 2. Generation Algorithm

```
GeneratorAtom::generate(seed, match, codeset):

1. Init author/grid from match dungeon layout
2. state = ArchitectState { isBlueOpen=true, keyBalance=0 }
3. occupied = OccupancyMap (empty)
4. Place entry room (simple DOORWAY atom, no gates)
5. Repeat N times (target: 6–10 atoms):
   a. candidates = AtomLibrary::getAll()
             .filter(a => state.canAccept(a.meta))
   b. atom = pick from candidates using seed+step
   c. embedded = embedWithRetry(atom, occupied, author, grid)
      - try 8–16 random (x,y) origins
      - first success wins; if all fail, skip atom
   d. compile(embedded, mutator)
   e. state.apply(atom.meta)
   f. mark rooms occupied
6. Place exit door on last atom's exit node
7. Assert state.keyBalance >= 0 (should always pass by construction)
```

No pathfinding. No validation loop. The dungeon is solvable by the invariants maintained in
step 5e.

---

## 3. Atom Library — Initial Catalogue

All atoms are 4–6 rooms. Room coordinates are (dx, dy) relative to origin.

### A. `ATOM_FREE_KEY` (2 rooms)
```
[Entry(0,0)] ──DOORWAY──> [KeyRoom(1,0)]
```
- meta: phaseSensitive=false, flipsPhase=false, keyRequired=0, keyProvided=1, keyConsumed=0
- Drops a key. Use to seed key economy at dungeon start.

### B. `ATOM_KEEPER_LINEAR` (4 rooms)
```
[Entry(0,0)] ──DOORWAY──> [KeyRoom(1,0)] ──DOORWAY──> [Gate(2,0)] ──KEEPER──> [Exit(3,0)]
```
- meta: keyRequired=0, keyProvided=1, keyConsumed=1 (net 0), phaseSensitive=false
- Player picks up key in room 1, uses it at gate to room 3.

### C. `ATOM_KEEPER_LOOP` (5 rooms)
```
[Entry(0,0)] ──DOORWAY──> [Gate(1,0)] ──KEEPER──> [Reward(2,0)]
                                  ↑
[KeyRoom(0,1)] ──DOORWAY──> [Detour(1,1)]
```
- meta: keyRequired=0, keyProvided=1, keyConsumed=1, phaseSensitive=false
- Player must detour south to find key, backtrack to gate.

### D. `ATOM_PHASE_FLIP` (3 rooms)
```
[Entry(0,0)] ──DOORWAY──> [Switch(1,0)] ──DOORWAY──> [Exit(2,0)]
```
- Switch room contains `ROLE_TOGGLER_SWITCH`.
- meta: flipsPhase=true, phaseSensitive=false, keyRequired=0, keyProvided=0, keyConsumed=0

### E. `ATOM_TOGGLER_CORRIDOR` (4 rooms)
```
[Entry(0,0)] ──TOGGLER_BLUE──> [Inner(1,0)] ──DOORWAY──> [Inner2(2,0)] ──DOORWAY──> [Exit(3,0)]
```
- meta: phaseSensitive=true, requiresBlueOpen=true, flipsPhase=false
- Only traversable when blue phase is active.

### F. `ATOM_TOGGLER_ORANGE_CORRIDOR` (4 rooms)
Same layout as E but uses `TOGGLER_ORANGE`.
- meta: requiresBlueOpen=false

### G. `ATOM_JAILER_BYPASS` (5 rooms)
```
[Entry(0,0)] ──JAILER──> [Direct(1,0)] ──DOORWAY──> [Exit(2,0)]
      └──DOORWAY──> [Bypass(0,1)] ──DOORWAY──> [Junction(1,1)]──┘
```
- Jailer blocks the short path; bypass route goes around.
- meta: phaseSensitive=false, keyRequired=0, keyProvided=0, keyConsumed=0

### H. `ATOM_SHIFTER_GUARDED` (5 rooms)
```
[Entry(0,0)] ──DOORWAY──> [KeyRoom(1,0)] ──SHIFTER──> [Locked(2,0)] ──DOORWAY──> [Exit(3,0)]
                                   └──────────────────────────── DOORWAY ─────────> [Return(2,1)]
```
- Shifter door moves the key. Return path from Exit back to Entry does NOT require the shifted key.
- meta: keyRequired=0, keyProvided=1, keyConsumed=1 (shifter absorbs key)
- Invariant: return path `Exit→Return→KeyRoom→Entry` uses only DOORWAYs.

---

## 4. Implementation Phases

### Phase 1 — Scaffolding (no generation yet)

1. Create `src/procgen/` directory.
2. Implement `Atom.hpp` (structs only, no logic).
3. Implement `ArchitectState.hpp` (canAccept + apply).
4. Write unit tests for `ArchitectState`: canAccept rejects key-debt states, phase mismatches.

Completion signal: `ArchitectState` unit tests pass.

### Phase 2 — Atom Library

1. Implement `AtomLibrary.hpp/.cpp` with atoms A–H above.
2. Write unit tests validating atom metadata consistency:
   - `keyProvided - keyConsumed == net key delta`
   - All KEEPER edges have a valid keyNodeId
   - SHIFTER atoms have a return-path-without-key annotation

Completion signal: all 8 atoms constructed, tests pass.

### Phase 3 — Spatial Embedding

1. Implement `OccupancyMap` as `Bitstick<DUNGEON_ROOM_COUNT>`.
2. Implement `AtomEmbedder::embed()`:
   - Validate relative positions form a connected planar graph
   - Check all `(x0+dx, y0+dy)` within grid bounds and unoccupied
   - Call `DungeonAuthor` to allocate rooms (investigate existing allocation API)
   - Return `EmbeddedAtom` with populated `roomIds[]`
3. Implement `AtomEmbedder::computeDirection(dx, dy)` — map relative offset to `Cardinal`.
   - **Risk**: verify Cardinal ordering against `Room::walls` index. Read `Cardinal.hpp` or equivalent.
4. Unit test with atom A (2 rooms), atom B (4 rooms), and a conflict scenario.

Completion signal: embedding tests place rooms without overlap.

### Phase 4 — Compiler

1. Implement `AtomCompiler::compile()` and `compileEdge()`.
2. For each `AtomDoor` variant, call the appropriate `DungeonMutator` method.
3. Key placement: when compileEdge encounters KEEPER, call mutator key-drop for `keyNodeId`.
4. Toggler switch placement: when `atom.meta.flipsPhase`, call `setupTogglerSwitch`.
5. Integration test: embed + compile atom B (`ATOM_KEEPER_LINEAR`) into a fresh `Match`.
   Inspect the resulting `Match.dungeon` walls and verify door types are correct.

Completion signal: integration test shows expected `DoorEnum` values in walls.

### Phase 5 — Generator

1. Implement `GeneratorAtom::generate()` per algorithm in Section 2.
2. Add `GENERATOR_ATOM` to `Generator.enum`.
3. Register `GeneratorAtom` in whatever flyweight/factory registers generators (check
   `GeneratorFlyweight.hpp` or equivalent — find how existing generators are registered).
4. Wire up in a test match (use `GENERATOR_TEST` as reference for how tests invoke generators).
5. Generate 10 dungeons with different seeds. For each:
   - `ArchitectState.keyBalance >= 0` after all atoms placed
   - No room appears in two atoms (occupancy check)
   - Dungeon has a reachable exit

Completion signal: 10/10 dungeons pass the three checks above.

### Phase 6 — Iteration

- Add more atoms (keeper+toggler combos, multi-key sequences).
- Tune selection weights so dungeons vary meaningfully.
- Add debug output: print atom sequence + phase/key trace per generation.
- Replace one existing test generator with `GENERATOR_ATOM` once confident.

---

## 5. Integration Risks & Mitigations

### Risk 1 — Cardinal direction convention unknown
DungeonMutator methods likely take a `Cardinal` direction. The mapping from `(dx,dy)` to
`Cardinal` must match `Room::walls[0..3]` indexing.

**Mitigation**: Read `src/room/Room.hpp` and `Cardinal.hpp` (or equivalent enum) before
implementing `AtomEmbedder::computeDirection`. Write a static assert or unit test that places
two known-adjacent rooms and checks the resulting `Wall::adjacent`.

### Risk 2 — DungeonAuthor room allocation API unclear
The plan assumes `DungeonAuthor` exposes a way to allocate rooms at specific (x,y,z,t) coords.
The actual API may differ.

**Mitigation**: Read `DungeonAuthor.hpp` fully in Phase 3. If it doesn't expose direct
allocation, use `LayoutGrid::getIndex(x, y, z, t)` directly and call `DungeonMutator::setRoom`
to initialize rooms.

### Risk 3 — Toggler globality causes phase drift across atoms
If atoms don't consistently exit in the same phase they entered, the sequence accumulates
phase drift and later atoms get wrong phase assumptions.

**Mitigation**: Atoms that are phase-neutral have `flipsPhase=false`. The only atom that
changes phase is `ATOM_PHASE_FLIP`. `ArchitectState::apply()` tracks the transition. The
generator only selects phase-sensitive atoms when `state.isBlueOpen` matches
`meta.requiresBlueOpen`. This is enforced in `canAccept`.

### Risk 4 — Shifter atoms break return paths
A shifter consumes or relocates a key. If the atom's return path also requires that key,
the dungeon becomes one-way.

**Mitigation**: Shifter atom definitions must annotate a return path that uses only DOORWAYs.
The atom library unit tests (Phase 2) validate this by inspecting edge door types on the
return route. The `AtomCompiler` does not need to re-validate at compile time—correctness is
enforced at definition time.

### Risk 5 — Key debt across atom boundaries
An atom can consume keys that were placed by a previous atom. If those keys were already
spent, or the sequence places a consuming atom before a providing atom, the dungeon is
unwinnable.

**Mitigation**: `ArchitectState::keyBalance` tracks the running total. `canAccept` rejects
any atom where `keyBalance + keyProvided - keyRequired < 0`. The generator always calls
`canAccept` before placing. This eliminates key debt by construction.

### Risk 6 — Embedding failures at high atom counts
With 8+ atoms in a 96-room dungeon, the spatial search may fail often if atoms are large.

**Mitigation**: Keep atoms ≤5 rooms. Use retry with 16 random origins per atom. If embedding
still fails after retries, skip that atom and try the next candidate. The generator does not
require a fixed atom count — it fills what fits.

---

## 6. File Checklist

**New files:**
```
src/procgen/Atom.hpp
src/procgen/AtomLibrary.hpp
src/procgen/AtomLibrary.cpp
src/procgen/ArchitectState.hpp
src/procgen/AtomEmbedder.hpp
src/procgen/AtomEmbedder.cpp
src/procgen/AtomCompiler.hpp
src/procgen/AtomCompiler.cpp
src/procgen/GeneratorAtom.hpp
src/procgen/GeneratorAtom.cpp
```

**Modified files:**
```
src/generator/Generator.enum        — add GENERATOR_ATOM
src/generator/GeneratorFlyweight.*  — register GeneratorAtom (if flyweight exists)
```

**Test files:**
```
test/procgen/ArchitectStateTest.cpp
test/procgen/AtomLibraryTest.cpp
test/procgen/AtomEmbedderTest.cpp
test/procgen/AtomCompilerTest.cpp
test/procgen/GeneratorAtomTest.cpp
```

---

## 7. What Success Looks Like

A correct implementation will:
- Generate a dungeon with zero pathfinding calls
- Never produce a `keyBalance < 0` state
- Never produce a phase-mismatch (phase-sensitive atom placed in wrong phase)
- Produce puzzles where players must backtrack, manage keys, and time phase flips
- Complete generation in under 1ms

A correct implementation will NOT:
- Run A* or BFS to validate the dungeon after generation
- Allow atoms to be placed without checking `ArchitectState`
- Hard-code room indices — all placement goes through `DungeonAuthor` + `LayoutGrid`
- Simulate player movement during generation
