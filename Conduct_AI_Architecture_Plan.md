# Conduct-Based AI Architecture Plan

## Goal

Replace creature-specific state machines with a reusable Conduct system composed of independent behavior modules.

## ConductMemory

```cpp
struct ConductMemory {
    FsmEnum state = FSM_NIL;

    int targetCharacterId = -1;
    int targetObjectId = -1;
    int targetRoomId = -1;
};
```

All behaviors share the same memory layout.

## Conduct Component

```cpp
struct Conduct {
    Array<ConductMemory, CONDUCT_COUNT> memory;
};
```

Every Character owns a Conduct instance.

## Conduct.enum

```cpp
CONDUCT_NIL,
CONDUCT_WANDER,
CONDUCT_HUNT,
CONDUCT_PICKPOCKET,
CONDUCT_DEPOSIT,
CONDUCT_FLEE,
CONDUCT_COUNT
```

The enum generation system should automatically expand:

```cpp
enum ConductEnum {
#include "Conduct.enum"
};
```

and

```cpp
Array<ConductMemory, CONDUCT_COUNT>
```

## Character Layout

```cpp
struct Character {
    Conduct conduct;
};
```

No creature-specific AI structures should exist.

## Behavior Execution

Each behavior receives its own ConductMemory:

```cpp
struct BehaviorFlyweight {
    const char* name;

    int score(
        ActivationContext&,
        ConductMemory&) const;

    bool activate(
        ActivationContext&,
        ConductMemory&) const;
};
```

Runtime:

```cpp
for each behavior
{
    if (trait allows behavior)
    {
        evaluate score();
    }
}

activate highest scoring behavior;
```

## Example Conduct Usage

### Wander

Uses:

```cpp
state
```

### Hunt

Uses:

```cpp
targetCharacterId
```

### Pickpocket

Uses:

```cpp
targetCharacterId
```

### Deposit

Uses:

```cpp
targetObjectId
```

### Flee

Uses:

```cpp
targetCharacterId
```

## Trait Mapping

```cpp
TRAIT_WANDERER        -> CONDUCT_WANDER
TRAIT_HUNTER          -> CONDUCT_HUNT
TRAIT_PICKPOCKETER    -> CONDUCT_PICKPOCKET
TRAIT_ITEM_SQUIRRELER -> CONDUCT_DEPOSIT
TRAIT_FLEEING         -> CONDUCT_FLEE
```

## Monkey Configuration

```cpp
TRAIT_WANDERER
TRAIT_PICKPOCKETER
TRAIT_ITEM_SQUIRRELER
```

Gains:

- Wander
- Pickpocket
- Deposit

No MonkeyAI class required.

## Serialization

Conduct is serialized as part of Character:

```cpp
serializer.value(conduct.memory);
```

## Deserialization

```cpp
deserializer.value(conduct.memory);
```

No behavior-specific restoration logic.

## API Visibility

Conduct should remain internal.

Do not expose:

```cpp
conduct
targetCharacterId
targetObjectId
targetRoomId
state
```

through ApiCharacter.

## Advantages

- No creature-specific AI classes.
- Uniform memory layout.
- Trait-driven behavior composition.
- Stable serialization.
- Extensible behavior system.
- Hidden implementation details.
