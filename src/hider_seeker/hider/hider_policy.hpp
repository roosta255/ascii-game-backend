#pragma once
#include <nlohmann/json.hpp>

namespace hider_seeker {

// These functions read only a hider view.
// Cost goalCost returns for a goal the NPC cannot complete.
constexpr int GOAL_INFEASIBLE = -1;

// NPC-010: Returns true if the requirement is met in the view.
bool requirementMet(const nlohmann::json& view, const nlohmann::json& requirement);
// NPC-011: Returns true if the goal is supported by the NPC rules.
bool goalSupported(const nlohmann::json& goal);
// NPC-012: Returns the AP cost to achieve the goal, or GOAL_INFEASIBLE.
int goalCost(const nlohmann::json& view, const nlohmann::json& goal);

} // namespace hider_seeker