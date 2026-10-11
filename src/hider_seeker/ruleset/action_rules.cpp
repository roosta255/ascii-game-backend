#include "action_rules.hpp"
#include <cstdint>

namespace hider_seeker {

// Placeholder constants for AP costs and effects - TODO: replace with final values
// These are placeholders as the design doc only gives hedged examples
constexpr uint16_t PLACEHOLDER_AP_CONFIGURE_SERVER = 5;
constexpr uint16_t PLACEHOLDER_AP_SUPPRESS_ERROR = 3;
constexpr uint16_t PLACEHOLDER_AP_BOOST_CPU = 2;
constexpr uint16_t PLACEHOLDER_AP_ROTATE_ROUTE = 4;
constexpr uint16_t PLACEHOLDER_AP_PAY_GOAL = 10;

// Static table of action rules - populated with placeholder values
static const ActionRuleEntry ACTION_RULES[] = {
    {PLACEHOLDER_AP_CONFIGURE_SERVER},   // CONFIGURE_SERVER
    {PLACEHOLDER_AP_SUPPRESS_ERROR},     // SUPPRESS_ERROR
    {PLACEHOLDER_AP_BOOST_CPU},          // BOOST_CPU
    {PLACEHOLDER_AP_ROTATE_ROUTE},       // ROTATE_ROUTE
    {PLACEHOLDER_AP_PAY_GOAL}            // PAY_GOAL
};

RuleOutcome applyHiderAction(MatchState& match, uint8_t hiderId, HiderActionMode mode) {
    // Look up cost/effect from the backend's own table (never from submitted intent)
    const ActionRuleEntry& entry = ACTION_RULES[static_cast<uint8_t>(mode)];
    
    // Check if hider has sufficient AP
    if (match.hiders[hiderId].action_points < entry.ap_cost) {
        return RuleOutcome::REJECTED_INSUFFICIENT_RESOURCE;
    }

    // Deduct the AP cost from HiderState::action_points only when action is accepted and applied
    match.hiders[hiderId].action_points -= entry.ap_cost;

    // Apply the effect (placeholder - actual implementation would be defined by the ruleset)
    // For now, we just return accepted since no specific effects are defined in the design doc
    
    return RuleOutcome::ACCEPTED;
}

} // namespace hider_seeker