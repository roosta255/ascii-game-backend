#pragma once

#include "ConductEnum.hpp"
#include "TriggerEffect.hpp"
#include "TriggerMatch.hpp"

// Configuration for one TriggerWrapper instance stored in a BehaviorFlyweight event slot.
// The event type and required FSM state are implicit: they come from which
// BEHAVIOR_DECL entry and which BEHAVIOR_ON_*_DECL slot hold this config.
struct TriggerConfig {
    static constexpr int MAX_MATCHES = 4;
    static constexpr int MAX_EFFECTS = 8;

    ConductEnum conduct = CONDUCT_NIL;  // which ConductMemory slot to read/write

    TriggerMatch matches[MAX_MATCHES] = {};
    TriggerEffect effects[MAX_EFFECTS] = {};
};
