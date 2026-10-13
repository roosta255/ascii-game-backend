#pragma once

#include "../hider_seeker/domain/state.hpp"

namespace hider_seeker {

void regenerateForTick(MatchState& state);

// Placeholder constants pending BALANCE-001 tuning
constexpr uint8_t PLACEHOLDER_AP_REGEN_PER_TICK = 2;
constexpr uint8_t PLACEHOLDER_AP_CAP = 10;
constexpr uint8_t PLACEHOLDER_SEEKER_TOKEN_REGEN_PER_TICK = 2;

} // namespace hider_seeker