#pragma once
#include "../domain/state.hpp"
#include "../seeker/policy_interface.hpp"
#include "../tick/tick_types.hpp"

namespace hider_seeker {

constexpr uint8_t PLACEHOLDER_SEEKER_OPENS_PER_TICK = 1; // PLACEHOLDER pending BALANCE-001

void runSeekerTurn(MatchState& state, const iSeekerPolicy& policy, TickOutput& output);

} // namespace hider_seeker