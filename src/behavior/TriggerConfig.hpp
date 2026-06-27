#pragma once

#include "TriggerEffect.hpp"
#include "TriggerMatch.hpp"

// Configuration for one TriggerWrapper instance stored in a BehaviorFlyweight event slot.
// The event type, required FSM state, and active conduct slot are all implicit: they come
// from which BEHAVIOR_DECL entry, which BEHAVIOR_ON_*_DECL slot, and which ConductEnum
// was active in the dispatch loop when this trigger fired.
struct TriggerConfig {
    static constexpr int MAX_MATCHES = 4;
    static constexpr int MAX_EFFECTS = 8;

    TriggerMatch matches[MAX_MATCHES] = {};
    TriggerEffect effects[MAX_EFFECTS] = {};
    // Applied when any entry in effects returns false (e.g. a scan finds nothing).
    TriggerEffect failureEffects[MAX_EFFECTS] = {};
};
