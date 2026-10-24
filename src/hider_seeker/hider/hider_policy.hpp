#pragma once
#include <nlohmann/json.hpp>

namespace hider_seeker {

// These functions read only a hider view.
// Cost goalCost returns for a goal the NPC cannot complete.
constexpr int GOAL_INFEASIBLE = -1;

// One NPC decision. When has_move, intent is the move API's "intent" object
// ({"type": ...}, plus "server_id" for CONNECT) and goal_id the goal it works toward.
struct HiderNpcChoice {
    bool has_move = false;
    nlohmann::json intent;
    int goal_id = -1;
};

// NPC-020: Returns the move choice for a hider NPC given its view.
// Step 2 rules:
// - If a goal's requirements are all met, pay it (with CONNECT fallback if disconnected).
// - Otherwise, pick the cheapest feasible goal and map its first unmet requirement to an intent.
HiderNpcChoice chooseHiderMove(const nlohmann::json& view);

// NPC-010: Returns true if the requirement is met in the view.
bool requirementMet(const nlohmann::json& view, const nlohmann::json& requirement);
// NPC-011: Returns true if the goal is supported by the NPC rules.
bool goalSupported(const nlohmann::json& goal);
// NPC-012: Returns the AP cost to achieve the goal, or GOAL_INFEASIBLE.
int goalCost(const nlohmann::json& view, const nlohmann::json& goal);

} // namespace hider_seeker