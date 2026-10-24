#pragma once
#include <nlohmann/json.hpp>

namespace hider_seeker {

// Cost goalCost returns for a goal the NPC cannot complete.
constexpr int GOAL_INFEASIBLE = -1;

bool requirementMet(const nlohmann::json& view, const nlohmann::json& requirement);
bool goalSupported(const nlohmann::json& goal);
int goalCost(const nlohmann::json& view, const nlohmann::json& goal);

} // namespace hider_seeker