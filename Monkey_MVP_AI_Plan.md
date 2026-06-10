# Monkey MVP AI Plan

## Overview

The monkey is a trait-driven NPC that pickpockets the player, hides stolen goods in an unlocked chest, then wanders. Its behavior is composed from reusable capability traits, not a dedicated MonkeyAI class. The four states (Idle, AttemptPickpocket, HideLoot, Wander) are derived each turn from computed traits and inventory — no extra boolean state.

Exit pressure is created by requiring minimum food and coin counts before the player can leave the dungeon.

---

## Implementation Order

### Step 1 — Traits (`src/traits/Trait.enum`)

Add to the `--- CRITTER & ITEM ATTRIBUTES ---` section:

```cpp
TRAIT_DECL(HANDS,             ({ TRAIT_TYPE_SOURCE, TRAIT_TYPE_CAPABILITY, TRAIT_TYPE_CHARACTER }))
TRAIT_DECL(PICKPOCKETER,      ({ TRAIT_TYPE_SOURCE, TRAIT_TYPE_CAPABILITY, TRAIT_TYPE_CHARACTER }))
TRAIT_DECL(PICKPOCKETABLE,    ({ TRAIT_TYPE_SOURCE, TRAIT_TYPE_ATTRIBUTE,  TRAIT_TYPE_CHARACTER }))
TRAIT_DECL(ITEM_SQUIRRELER,   ({ TRAIT_TYPE_SOURCE, TRAIT_TYPE_CAPABILITY, TRAIT_TYPE_CHARACTER }))
TRAIT_DECL(HIDER,             ({ TRAIT_TYPE_SOURCE, TRAIT_TYPE_CAPABILITY, TRAIT_TYPE_CHARACTER }))
TRAIT_DECL(WANDERER,          ({ TRAIT_TYPE_SOURCE, TRAIT_TYPE_CAPABILITY, TRAIT_TYPE_CHARACTER }))
TRAIT_DECL(FLEEING,           ({ TRAIT_TYPE_SOURCE, TRAIT_TYPE_STATE,      TRAIT_TYPE_CHARACTER }))
```

Already present — do not re-add:
- `TRAIT_DEXTERITY` (line 7)
- `TRAIT_ACTION_READY` (line 99)

---

### Step 2 — Items (`src/inventory/Item.enum`)

Append after `ITEM_ARMOR`:

```cpp
ITEM_DECL(COIN, 99, iActivator, ({ TRAIT_ITEM_TRANSFERABLE }))
ITEM_DECL(FOOD,  9, iActivator, ({ TRAIT_ITEM_TRANSFERABLE }))
```

`COIN` stacks to 99 (currency). `FOOD` stacks to 9 (consumable). Both are transferable so they move between inventories via the existing `takeInventoryItem` / `giveInventoryItem` path.

---

### Step 3 — Behavior Events (`src/behavior/BehaviorEventEnum.hpp`)

Add two events before `BEHAVIOR_EVENT_COUNT`:

```cpp
BEHAVIOR_EVENT_PICKPOCKET,
BEHAVIOR_EVENT_DEPOSIT,
```

Then in `BehaviorFlyweight` (`src/behavior/BehaviorFlyweight.hpp` / `.cpp`):
- Add `Pointer<const iActivator> onPickpocket` and `onDeposit` fields alongside the existing `onMove`, `onAttack`, etc.
- Add `BEHAVIOR_ON_PICKPOCKET_DECL` and `BEHAVIOR_ON_DEPOSIT_DECL` sub-declaration macros in `Behavior.enum`, matching the existing `BEHAVIOR_ON_MOVE_DECL` pattern.
- Update `getActivatorForEvent` to dispatch the two new cases.

---

### Step 4 — Monkey Role (`src/role/Role.enum`)

Add after `SCORPION`:

```cpp
ROLE_DECL(MONKEY, 1, 2, 2, 0, ActivatorMonkey,
    ({ TRAIT_ACTOR, TRAIT_MOBILE, TRAIT_ORGANIC, TRAIT_FOGGY,
       TRAIT_HANDS, TRAIT_PICKPOCKETER, TRAIT_ITEM_SQUIRRELER,
       TRAIT_HIDER, TRAIT_WANDERER }))
```

- `moves = 2`, `actions = 2`: one action to pickpocket, one to deposit; two moves to reach targets.
- `TRAIT_PICKPOCKETABLE` is **not** a monkey source trait — it marks victims (e.g. the player's MINION/TINKER role gets it).

---

### Step 5 — Monkey Activator (`src/play/ActivatorMonkey.hpp` / `.cpp`)

Create `ActivatorMonkey : public iActivator`. Each turn it evaluates state from top to bottom and executes the first applicable action:

```
HideLoot  — monkey inventory contains a stolen item AND an unlocked chest exists in the room
              → move toward chest, deposit item (fire BEHAVIOR_EVENT_DEPOSIT), transition to Wander

AttemptPickpocket — TRAIT_PICKPOCKETER present AND adjacent character has TRAIT_PICKPOCKETABLE
                     AND that character lacks TRAIT_ACTION_READY
              → steal one FOOD or COIN item (fire BEHAVIOR_EVENT_PICKPOCKET), set targetChestId

Wander    — TRAIT_WANDERER present AND no stolen item in inventory
              → move to a random adjacent walkable cell

Idle      — none of the above applies
              → end turn immediately
```

State is derived per turn — no persistent `targetCharacterId` or `targetChestId` fields needed on `Character`. The monkey finds the nearest unlocked chest each turn via `match.dungeon.findChestByContainerId` (already used in `ActivatorLootChest`).

Pickpocket check uses `controller.getTraitsComputed(victimId).final` to read `TRAIT_ACTION_READY`, same pattern as the `isActor` check in `ActivatorLootChest:26`.

---

### Step 6 — Exit Gating (`src/door/ActivatorExitDungeon.cpp`)

Before the `subject.takeMove` call, check the exiting player's inventory:

```cpp
int coins = player.inventory.countStacks(ITEM_COIN);
int food  = player.inventory.countStacks(ITEM_FOOD);
if (codeset.addFailure(coins < EXIT_MIN_COINS || food < EXIT_MIN_FOOD, CODE_EXIT_INSUFFICIENT_RESOURCES)) {
    controller.addRequestLoggedEvent(activation, LoggedEvent{
        EVENT_EXIT_INSUFFICIENT_RESOURCES,
        { EventComponentKind::ROLE, (int)subject.role },
        {}, {}, -1
    });
    return;
}
```

Define `EXIT_MIN_COINS` and `EXIT_MIN_FOOD` as constants (start at 1 each for MVP). Add `CODE_EXIT_INSUFFICIENT_RESOURCES` to `src/enum/Code.enum` and `EVENT_EXIT_INSUFFICIENT_RESOURCES` to `src/event/Event.enum`.

`Inventory::countStacks(ItemEnum)` likely needs to be added — it iterates items and sums `stacks` for the matching type.

---

## MVP Trait Matrix

| Trait | Monkey | Player roles | Purpose |
|---|---|---|---|
| `TRAIT_HANDS` | source | — | gates AttemptPickpocket capability |
| `TRAIT_PICKPOCKETER` | source | — | marks a character as able to pickpocket |
| `TRAIT_PICKPOCKETABLE` | — | MINION, TINKER (add to their role) | marks a character as a valid victim |
| `TRAIT_ITEM_SQUIRRELER` | source | — | gates HideLoot capability |
| `TRAIT_HIDER` | source | — | gates chest-seek behavior |
| `TRAIT_WANDERER` | source | future NPCs | gates Wander behavior |
| `TRAIT_DEXTERITY` | source | — | already present; required for pickpocket to succeed |
| `TRAIT_ACTION_READY` | — | player-controlled | blocks pickpocket attempt when present |

---

## MVP Flow

```
Monkey adjacent to player
  └─ player lacks TRAIT_ACTION_READY?
       Yes → steal FOOD or COIN → HideLoot
                └─ find unlocked chest → deposit item → Wander
       No  → Idle

Player recovers stolen goods by looting the chest (existing ActivatorLootChest).

Player attempts exit → ActivatorExitDungeon checks inventory.
  Insufficient FOOD or COIN → blocked with EVENT_EXIT_INSUFFICIENT_RESOURCES.
```

---

## Out of Scope (Post-MVP)

- `BEHAVIOR_EVENT_STEAL`, `BEHAVIOR_EVENT_HIDE`, `BEHAVIOR_EVENT_WAIT`
- `TRAIT_FLEEING` and the Flee state
- Multi-room monkey navigation (MVP assumes monkey and chest are in the same room)
- Monkey animation keyframes for pickpocket and deposit actions
