#include "action_rules.hpp"
#include "../../data/Pointer.hpp"
#include <cstdint>

namespace hider_seeker {

// Placeholder constants for AP costs and effects - TODO: replace with final values
// These are placeholders as the design doc only gives hedged examples
constexpr uint16_t PLACEHOLDER_AP_ADD_SERVER = 0; // No extra cost - 1 LOA is already charged by loa_rules.cpp
constexpr uint16_t PLACEHOLDER_AP_REMOVE_SERVER = 0; // No extra cost - 1 LOA is already charged by loa_rules.cpp
constexpr uint16_t PLACEHOLDER_AP_CONFIGURE_SERVER = 5;
constexpr uint16_t PLACEHOLDER_AP_SUPPRESS_ERROR = 3;
constexpr uint16_t PLACEHOLDER_AP_BOOST_CPU = 2;
constexpr uint16_t PLACEHOLDER_AP_ROTATE_ROUTE = 4;
constexpr uint16_t PLACEHOLDER_AP_PAY_GOAL = 10;

// Placeholder effect parameters - TODO: replace with final ruleset values.
// The design doc only gives hedged examples, so each mode's effect is a single
// signed delta on one metric of the acting hider's occupied server.
// ADD/REMOVE_SERVER carry no metric effect here: their structural effect
// (slot activation / removal) is applied by applyAddServer / applyRemoveServer.
// CONFIGURE_SERVER and PAY_GOAL affect goal state, which is not modelled by
// this slice, so they also carry no metric effect yet.
constexpr int16_t PLACEHOLDER_EFFECT_SUPPRESS_ERROR = -10; // ERRORS
constexpr int16_t PLACEHOLDER_EFFECT_BOOST_CPU = 15;       // CPU
constexpr int16_t PLACEHOLDER_EFFECT_ROTATE_ROUTE = 5;     // NETWORK

// Static table of action rules, indexed by HiderActionMode - placeholder values
static const ActionRuleEntry ACTION_RULES[] = {
    {PLACEHOLDER_AP_ADD_SERVER,        ActionEffectMetric::NONE,    0},                               // ADD_SERVER (index 0)
    {PLACEHOLDER_AP_REMOVE_SERVER,     ActionEffectMetric::NONE,    0},                               // REMOVE_SERVER (index 1)
    {PLACEHOLDER_AP_CONFIGURE_SERVER,  ActionEffectMetric::NONE,    0},                               // CONFIGURE_SERVER (index 2)
    {PLACEHOLDER_AP_SUPPRESS_ERROR,    ActionEffectMetric::ERRORS,  PLACEHOLDER_EFFECT_SUPPRESS_ERROR}, // SUPPRESS_ERROR (index 3)
    {PLACEHOLDER_AP_BOOST_CPU,         ActionEffectMetric::CPU,     PLACEHOLDER_EFFECT_BOOST_CPU},     // BOOST_CPU (index 4)
    {PLACEHOLDER_AP_ROTATE_ROUTE,      ActionEffectMetric::NETWORK, PLACEHOLDER_EFFECT_ROTATE_ROUTE},  // ROTATE_ROUTE (index 5)
    {PLACEHOLDER_AP_PAY_GOAL,          ActionEffectMetric::NONE,    0}                                // PAY_GOAL (index 6)
};

namespace {

Pointer<uint8_t> metricField(MetricFrame& frame, ActionEffectMetric metric) {
    switch (metric) {
        case ActionEffectMetric::CPU:     return Pointer<uint8_t>(frame.cpu);
        case ActionEffectMetric::MEMORY:  return Pointer<uint8_t>(frame.memory);
        case ActionEffectMetric::DISK:    return Pointer<uint8_t>(frame.disk);
        case ActionEffectMetric::NETWORK: return Pointer<uint8_t>(frame.network);
        case ActionEffectMetric::HEAT:    return Pointer<uint8_t>(frame.heat);
        case ActionEffectMetric::ERRORS:  return Pointer<uint8_t>(frame.errors);
        case ActionEffectMetric::USERS:   return Pointer<uint8_t>(frame.users);
        case ActionEffectMetric::NONE:    break;
    }
    return Pointer<uint8_t>();
}

// Applies entry's metric effect to the acting hider's occupied server.
// Placeholder rule: if the hider occupies no active server, the metric effect
// has no target and is skipped (the AP cost is still paid).
void applyActionEffect(MatchState& match, uint8_t hiderId, const ActionRuleEntry& entry) {
    const uint8_t serverId = match.hiders[hiderId].server_id;
    if (serverId >= MAX_SERVERS || !match.servers[serverId].active) {
        return;
    }
    metricField(match.servers[serverId].current, entry.effect_metric).access([&entry](uint8_t& field) {
        int32_t value = static_cast<int32_t>(field) + entry.effect_delta;
        if (value < 0) value = 0;
        if (value > 255) value = 255;
        field = static_cast<uint8_t>(value);
    });
}

} // namespace

RuleOutcome applyHiderAction(MatchState& match, uint8_t hiderId, HiderActionMode mode) {
    // Look up cost/effect from the backend's own table (never from submitted intent)
    const ActionRuleEntry& entry = ACTION_RULES[static_cast<uint8_t>(mode)];
    
    // Check if hider has sufficient AP
    if (match.hiders[hiderId].action_points < entry.ap_cost) {
        return RuleOutcome::REJECTED_INSUFFICIENT_RESOURCE;
    }

    // Deduct the AP cost from HiderState::action_points only when action is accepted and applied
    match.hiders[hiderId].action_points -= entry.ap_cost;

    // Apply the mode's effect, taken from the same backend table entry
    applyActionEffect(match, hiderId, entry);

    return RuleOutcome::ACCEPTED;
}

} // namespace hider_seeker