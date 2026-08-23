#pragma once

#include "Array.hpp"
#include "TriggerEffect.hpp"

// Effects executed when an FSM transitions into or out of a BehaviorFlyweight state.
// No matches field — transitions are unconditional.
struct TransitionConfig {
    static constexpr int MAX_EFFECTS = 8;
    Array<TriggerEffect, MAX_EFFECTS> effects = {};
};
