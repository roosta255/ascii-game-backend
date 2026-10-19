#pragma once

#include "../domain/state.hpp"
#include "policy_interface.hpp"
#include "../tick/tick_types.hpp"
#include "feature_extract.hpp"

namespace hider_seeker {

void runSeekerTurn(MatchState& state, const iSeekerPolicy& policy, TickOutput& output);

constexpr uint8_t PLACEHOLDER_SEEKER_OPENS_PER_TICK = 1; // PLACEHOLDER pending BALANCE-001

} // namespace hider_seeker