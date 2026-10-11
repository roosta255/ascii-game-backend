#include "action_rules.hpp"
#include <cstdint>

namespace hider_seeker {

// Placeholder constants for AP costs and effects - TODO: replace with final values
// These are placeholders as the design doc only gives hedged examples
constexpr uint16_t PLACEHOLDER_AP_ADD_SERVER = 5;
constexpr uint16_t PLACEHOLDER_AP_REMOVE_SERVER = 3;
constexpr uint16_t PLACEHOLDER_AP_CONFIGURE_SERVER = 5;
constexpr uint16_t PLACEHOLDER_AP_SUPPRESS_ERROR = 3;
constexpr uint16_t PLACEHOLDER_AP_BOOST_CPU = 2;
constexpr uint16_t PLACEHOLDER_AP_ROTATE_ROUTE = 4;
constexpr uint16_t PLACEHOLDER_AP_PAY_GOAL = 10;

// Placeholder effect parameters - these would be defined by ruleset data
// For now, they are placeholders to match the implementation plan's stated design
constexpr int16_t PLACEHOLDER_EFFECT_ADD_SERVER_1 = 10;
constexpr int16_t PLACEHOLDER_EFFECT_ADD_SERVER_2 = 5;
constexpr int16_t PLACEHOLDER_EFFECT_REMOVE_SERVER_1 = -5;
constexpr int16_t PLACEHOLDER_EFFECT_REMOVE_SERVER_2 = 3;
constexpr int16_t PLACEHOLDER_EFFECT_CONFIGURE_SERVER_1 = 0;
constexpr int16_t PLACEHOLDER_EFFECT_CONFIGURE_SERVER_2 = 0;
constexpr int16_t PLACEHOLDER_EFFECT_SUPPRESS_ERROR_1 = -2;
constexpr int16_t PLACEHOLDER_EFFECT_SUPPRESS_ERROR_2 = 1;
constexpr int16_t PLACEHOLDER_EFFECT_BOOST_CPU_1 = 15;
constexpr int16_t PLACEHOLDER_EFFECT_BOOST_CPU_2 = 0;
constexpr int16_t PLACEHOLDER_EFFECT_ROTATE_ROUTE_1 = 0;
constexpr int16_t PLACEHOLDER_EFFECT_ROTATE_ROUTE_2 = 0;
constexpr int16_t PLACEHOLDER_EFFECT_PAY_GOAL_1 = -10;
constexpr int16_t PLACEHOLDER_EFFECT_PAY_GOAL_2 = 0;

// Static table of action rules - populated with placeholder values
static const ActionRuleEntry ACTION_RULES[] = {
    {PLACEHOLDER_AP_ADD_SERVER, PLACEHOLDER_EFFECT_ADD_SERVER_1, PLACEHOLDER_EFFECT_ADD_SERVER_2},         // ADD_SERVER (index 0)
    {PLACEHOLDER_AP_REMOVE_SERVER, PLACEHOLDER_EFFECT_REMOVE_SERVER_1, PLACEHOLDER_EFFECT_REMOVE_SERVER_2},      // REMOVE_SERVER (index 1) 
    {PLACEHOLDER_AP_CONFIGURE_SERVER, PLACEHOLDER_EFFECT_CONFIGURE_SERVER_1, PLACEHOLDER_EFFECT_CONFIGURE_SERVER_2},   // CONFIGURE_SERVER (index 2)
    {PLACEHOLDER_AP_SUPPRESS_ERROR, PLACEHOLDER_EFFECT_SUPPRESS_ERROR_1, PLACEHOLDER_EFFECT_SUPPRESS_ERROR_2},     // SUPPRESS_ERROR (index 3)
    {PLACEHOLDER_AP_BOOST_CPU, PLACEHOLDER_EFFECT_BOOST_CPU_1, PLACEHOLDER_EFFECT_BOOST_CPU_2},          // BOOST_CPU (index 4)
    {PLACEHOLDER_AP_ROTATE_ROUTE, PLACEHOLDER_EFFECT_ROTATE_ROUTE_1, PLACEHOLDER_EFFECT_ROTATE_ROUTE_2},       // ROTATE_ROUTE (index 5)
    {PLACEHOLDER_AP_PAY_GOAL, PLACEHOLDER_EFFECT_PAY_GOAL_1, PLACEHOLDER_EFFECT_PAY_GOAL_2}            // PAY_GOAL (index 6)
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
    // For now, we apply placeholder effects based on the mode
    switch (mode) {
        case HiderActionMode::ADD_SERVER:
            // Placeholder effect: increase server count and modify some resource
            match.server_count += entry.effect_param1;
            break;
        case HiderActionMode::REMOVE_SERVER:
            // Placeholder effect: decrease server count and modify some resource
            match.server_count += entry.effect_param1;
            break;
        case HiderActionMode::CONFIGURE_SERVER:
            // Placeholder effect: no direct state change for now
            break;
        case HiderActionMode::SUPPRESS_ERROR:
            // Placeholder effect: reduce error count or similar
            break;
        case HiderActionMode::BOOST_CPU:
            // Placeholder effect: increase CPU performance metric
            break;
        case HiderActionMode::ROTATE_ROUTE:
            // Placeholder effect: modify routing parameters
            break;
        case HiderActionMode::PAY_GOAL:
            // Placeholder effect: reduce goal count or similar
            break;
    }
    
    return RuleOutcome::ACCEPTED;
}

} // namespace hider_seeker