# Field System Implementation Plan

## Purpose

Implement a generic dungeon Field system for derived environmental states.

Fields represent stable environmental information derived from world state.

Examples:

- Heat
- Oxygen
- Smoke
- Poison gas
- Light
- Magic influence
- Sound
- Enemy influence

The system should support puzzle mechanics, not realistic simulation.

Primary goals:

- Deterministic behavior
- Event-driven recomputation
- No oscillation
- Minimal CPU cost
- Data-driven configuration
- Reusable field algorithms

---

# Core Architecture

The system consists of:

```
World State
    |
    v
Field Controller
    |
    +-- Field Definitions (flyweights)
    |
    +-- Source Providers
    |
    +-- Field Resolvers
    |
    +-- Derived Flags
    |
    v
Gameplay Queries
```

World state is authoritative.

Fields are derived state.

Characters, objects, doors, and rooms never directly modify fields.

---

# Room Limit Optimization

Maximum rooms = 64.

Use bitsets extensively.

## Room Values

Each field stores:

```cpp
std::array<int16_t, 64> values;
```

Example:

```
Heat:
[10, 20, 0, 5...]

Oxygen:
[100, 90, 40, 100...]
```

Fields are indexed by RoomId.

---

## Dirty Tracking

Use:

```cpp
uint64_t dirtyRooms;
```

Each bit represents a room requiring recomputation.

Example:

```cpp
dirtyRooms |= (1ULL << roomId);
```

Benefits:

- O(1) updates
- Cache friendly
- Cheap room invalidation

---

## Room Flags

Derived conditions should also use bitsets.

Example:

```cpp
uint64_t smokeRooms;
uint64_t hypoxiaRooms;
uint64_t coldRooms;
```

Query:

```cpp
if (hypoxiaRooms & (1ULL << roomId))
{
    ApplyHypoxia();
}
```

---

# Field Structure

```cpp
struct Field
{
    FieldEnum id;

    std::array<int16_t, 64> values;

    uint64_t dirtyRooms;

    void recompute(World& world);

    int16_t get(RoomId room);
};
```

Fields should not store permanent sources.

Sources are discovered from the world when recomputing.

---

# Source Collection

Do not maintain duplicate source state.

The world already contains:

- Characters
- Objects
- Rooms
- Doors
- Items
- Effects

Fields query those systems.

Example:

```
Recompute Heat

    CollectHeatSources()

        Character with ON_FIRE
        Torch object
        Lava room

    Resolve heat field

    Generate flags
```

This avoids synchronization bugs.

---

# Field Lifecycle

## 1. Something changes

Examples:

- Character catches fire
- Door opens
- Vent activates
- Object destroyed

The responsible system marks fields dirty.

Example:

```cpp
dirtyFields |= FieldEnum::HEAT;
dirtyFields |= FieldEnum::SMOKE;
```

---

## 2. Field requested

Example:

```cpp
fieldController.get(
    FieldEnum::SMOKE,
    roomId
);
```

If dirty:

```
recompute()
```

Otherwise return cached value.

---

## 3. Recompute

Generic flow:

```cpp
RecomputeField(field)
{
    Clear previous values;

    Collect sources;

    Resolve field;

    Clamp values;

    Generate derived flags;

    Clear dirty state;
}
```

---

# Data Driven Field Configuration

Use Field.enum X-macro definitions to generate:

- Field enum
- Flyweights
- Metadata
- Resolver selection
- Threshold definitions

---

# Field Macro Definition

Example:

```cpp
FIELD(
    HEAT,
    int16_t,
    DISTANCE_FIELD,
    HEAT_SOURCE_PROVIDER,
    CHARACTER | OBJECT | ROOM,
    ROOM,
    -100,
    100,
    HEAT_FLAGS
)
```

Configuration fields:

## Name

Generated enum identifier.

Example:

```
HEAT
```

---

## Value Type

Storage type.

Recommended:

```
int16_t
```

for all scalar fields.

---

## Resolver

Selects generic resolution algorithm.

Examples:

```
DISTANCE_FIELD
ROOM_ACCUMULATION
RELAXATION
ROOM_EQUALIZATION
CUSTOM
```

---

## Source Provider

Defines how sources are collected.

Examples:

```
HEAT_SOURCE_PROVIDER
SMOKE_SOURCE_PROVIDER
OXYGEN_SOURCE_PROVIDER
```

---

## Invalidation Mask

Defines what events can make the field dirty.

Examples:

```
CHARACTER
OBJECT
ROOM
TOPOLOGY
TURN
```

---

## Domain

Where the field exists.

Initial:

```
ROOM
```

Future:

```
TILE
GRAPH_EDGE
CHARACTER
```

---

## Min / Max

Clamp values.

Examples:

Heat:

```
-100 to 100
```

Oxygen:

```
0 to 100
```

Smoke:

```
0 to 255
```

---

# Field Resolvers

Resolvers are reusable algorithms.

Do not create a unique resolver for every field.

---

## ROOM_ACCUMULATION

Use for values that are directly generated.

Example:

Oxygen.

```
room oxygen =
base oxygen
+ vents
- consumption
```

---

## DISTANCE_FIELD

Use for spatial influence.

Example:

Heat.

Sources propagate through rooms.

```
source strength
    |
    v
decrease with distance
```

---

## RELAXATION

Use for gradual spreading.

Example:

Smoke.

Neighbor rooms slowly approach equilibrium.

Important rules:

- Use buffered updates
- Limit transfer rate
- Never exceed available quantity

---

## ROOM_EQUALIZATION

Use when rooms combine.

Example:

Opening an airlock.

Two rooms average their atmospheric values.

---

# Initial Fields

## Heat

Resolver:

```
DISTANCE_FIELD
```

Sources:

- Fire
- Lava
- Heat objects

Flags:

```
HEAT_STROKE

UNCOMBUSTIBLE_COLD
```

---

## Oxygen

Resolver:

```
ROOM_ACCUMULATION
```

Sources:

- Vents
- Fire consumption
- Sealed rooms

Flags:

```
OXYGENATED

AHYPOXEMIA

UNCOMBUSTIBLE_DEOXYGENATED
```

---

## Smoke

Resolver:

```
RELAXATION
```

Sources:

- Fire
- Smoke objects

Flags:

```
SMOKE_INHALATION
```

---

# Character Interaction

Characters do not query scalar values.

They query derived room flags.

Example:

```
Room:
    AHYPOXEMIA

Character:
    hypoxiaTimer += turn
```

Environmental duration belongs to characters.

---

# Atmospheric Design Rules

Do not implement:

- Fluid dynamics
- Velocity vectors
- Pressure simulation
- Navier-Stokes
- Per-tile gas simulation
- Continuous airflow

Use:

- Stable fields
- Directed relaxation
- Room-level state
- Discrete flags

---

# Future Extensions

## Wind / Gas Velocity

Do not add velocity fields.

Use directional bias.

Example:

```cpp
RoomEdge
{
    RoomId a;
    RoomId b;

    float conductance;
    float bias;
}
```

This enables:

- Fans
- Drafts
- Chimneys
- Magic wind

without instability.

---

## Additional Fields

Add only when they create new puzzles.

Candidates:

### Water

Interactions:

- Extinguishes fire
- Conducts electricity
- Changes terrain

### Electricity

Interactions:

- Powers machines
- Interacts with water

### Poison Gas

Interactions:

- Area denial
- Enemy manipulation

### Steam

Interactions:

- Heat + water interaction
- Vision obstruction

---

# Implementation Order

1. Create Field enum/flyweight generation.
2. Create FieldController.
3. Implement room value storage.
4. Implement dirty field tracking.
5. Implement source providers.
6. Implement generic flag threshold evaluation.
7. Implement Heat.
8. Implement Oxygen.
9. Implement Smoke.
10. Add directional room-edge bias if needed.

---

# Final Design Principle

Fields are not simulations.

Fields are cached answers to:

"What is the current environmental state of this part of the dungeon?"

The world changes.
Fields recompute.
Gameplay reacts.

This keeps the dungeon systemic, deterministic, and easy to extend.