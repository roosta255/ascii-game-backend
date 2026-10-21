#pragma once

#include "../domain/state.hpp"
#include "rule_types.hpp"

namespace hider_seeker {

constexpr uint8_t GOAL_STATUS_OPEN = 0;
constexpr uint8_t GOAL_STATUS_COMPLETED = 1;
constexpr uint16_t PLACEHOLDER_HIDER_REWARD_TARGET = 3; // PLACEHOLDER pending BALANCE-001

// Whether one requirement holds in this state.
bool requirementMet(MatchState& state, const Requirement& requirement);
// Whether the goal has at least one requirement and all of its first
// requirement_count requirements are met.
bool goalRequirementsMet(MatchState& state, const GoalState& goal);
// The lowest goal index that is active, open, and has all requirements met; -1 if none.
int completableGoal(MatchState& state);
// Sum of reward_amount over active goals with status GOAL_STATUS_COMPLETED.
uint16_t teamReward(const MatchState& state);
// Clears every goal slot, then writes the four placeholder goals below.
void seedPlaceholderGoals(MatchState& state);

} // namespace hider_seeker