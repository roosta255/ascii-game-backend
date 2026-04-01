# Activator Wrapper Enhancement Plan

## Overview

This document outlines the required enhancements to evolve the current `ActivatorWrapper` system into a scalable, data-driven **interaction rule engine**.

The goal is to support complex interactions such as:
- key vs lock
- lockpick + dexterity vs lock
- future tools, spells, and environmental interactions

without hardcoding logic into items or doors.

---

## Current System Summary

The existing `WrapperConfig` supports:
- Trait requirements/restrictions
- Item costs
- Action/move costs
- Effect execution
- Failure logging

However, it treats all conditions as a single flat set and lacks the ability to model interactions between **actor, tool, and target**.

---

## Core Design Goal

All interactions should be modeled as:

> **(actor + tool + target) → rule → outcome**

Each wrapper should represent a **complete interaction rule**, not a partial condition.

---

## Required Enhancements

### 1. Split Trait Conditions by Entity

#### Problem
Current system uses:

```
TraitBits required;
TraitBits restricted;
```

This cannot distinguish between actor, tool, and target.

#### Solution

```
TraitBits actorRequired;
TraitBits actorRestricted;

TraitBits toolRequired;
TraitBits toolRestricted;

TraitBits targetRequired;
TraitBits targetRestricted;
```

#### Benefit
Enables rules like:
- "lockpick + mechanical lock"
- "key + specific door"

---

### 2. Separate Matching vs Requirements

#### Problem
"required" is used for both:
- determining if a rule applies
- determining if a rule succeeds

#### Solution

```
// Matching (rule selection)
TraitBits matchActor;
TraitBits matchTool;
TraitBits matchTarget;

// Requirements (success conditions)
TraitBits requireActor;
TraitBits restrictActor;
```

#### Benefit
Allows:
- Rule matches → but fails with meaningful error
- Proper feedback for missing stats (e.g., dexterity)

---

### 3. Add Rule Priority / Specificity

#### Problem
Multiple rules may match with no defined resolution order.

#### Solution

```
int priority;
```

Optionally compute specificity score based on:
- number of matching traits
- presence of specific IDs

#### Benefit
Ensures deterministic rule selection.

---

### 4. Add Target and Tool Identity Matching

#### Problem
Traits alone cannot express unique relationships.

#### Solution

```
EntityId targetId;
ItemEnum toolId;
```

#### Benefit
Supports:
- specific keys for specific doors
- quest-based interactions

---

### 5. Split Success and Failure Effects

#### Problem
Only a single `effects` list exists.

#### Solution

```
ActivatorEffect onSuccess[MAX_EFFECTS];
ActivatorEffect onFail[MAX_EFFECTS];
```

#### Benefit
Enables:
- trap triggers
- item degradation
- failure consequences

---

## Recommended Implementation Order

1. Split trait conditions (actor/tool/target)
2. Add match vs requirement separation
3. Implement rule priority system
4. Add identity matching (IDs)
5. Introduce fallback rule

Remaining enhancements can be added incrementally.

---

## Final Note

The key architectural shift is:

> Interaction logic lives in **rules**, not in items or targets.

This allows the system to scale cleanly as new mechanics and content are introduced.

