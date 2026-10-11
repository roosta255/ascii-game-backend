#pragma once

#include "rule_types.hpp"
#include "../domain/state.hpp"

namespace hider_seeker {

enum class HiderActionMode : uint8_t {
    ADD_SERVER,         // index 0
    REMOVE_SERVER,      // index 1
    CONFIGURE_SERVER,   // index 2
    SUPPRESS_ERROR,     // index 3
    BOOST_CPU,          // index 4
    ROTATE_ROUTE,       // index 5  
    PAY_GOAL            // index 6
};

struct ActionRuleEntry {
    uint16_t ap_cost;
    // Effect parameters for each action mode - these would be defined by ruleset data
    // For now, they are placeholders to match the implementation plan's stated design
    int16_t effect_param1;  // Placeholder for first effect parameter
    int16_t effect_param2;  // Placeholder for second effect parameter
};

RuleOutcome applyHiderAction(MatchState&, uint8_t hiderId, HiderActionMode mode);

} // namespace hider_seeker