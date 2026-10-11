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

// Which MetricFrame field of the acting hider's occupied server an action's
// effect modifies. NONE means the mode has no per-tick metric effect in this
// table (e.g. ADD/REMOVE_SERVER, whose structural effect lives in loa_rules).
enum class ActionEffectMetric : uint8_t {
    NONE,
    CPU,
    MEMORY,
    DISK,
    NETWORK,
    HEAT,
    ERRORS,
    USERS
};

// AP cost and effect parameters for one HiderActionMode (all integer fields).
// The backend derives both cost and effect of an accepted action from this
// entry, never from the submitted intent (AP-001).
struct ActionRuleEntry {
    uint16_t ap_cost;
    ActionEffectMetric effect_metric; // metric modified on the hider's occupied server
    int16_t effect_delta;             // signed amount added to that metric (clamped to 0..255)
};

RuleOutcome applyHiderAction(MatchState&, uint8_t hiderId, HiderActionMode mode);

} // namespace hider_seeker