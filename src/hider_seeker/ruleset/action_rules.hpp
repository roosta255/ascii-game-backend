#pragma once

#include "rule_types.hpp"
#include "../domain/state.hpp"

namespace hider_seeker {

enum class HiderActionMode : uint8_t {
    ADD_SERVER,
    REMOVE_SERVER,
    CONFIGURE_SERVER,
    SUPPRESS_ERROR,
    BOOST_CPU,
    ROTATE_ROUTE,
    PAY_GOAL
};

struct ActionRuleEntry {
    uint16_t ap_cost;
    // Effect parameters would go here, but are not specified in the design doc
    // For now, we'll leave them as placeholders
};

RuleOutcome applyHiderAction(MatchState&, uint8_t hiderId, HiderActionMode mode);

} // namespace hider_seeker