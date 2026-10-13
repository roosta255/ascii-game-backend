#pragma once

#include "../domain/state.hpp"

namespace hider_seeker {

// Placeholder regeneration constants - TODO(BALANCE-001): replace with final
// tuned values. The design doc only gives hedged examples, the same way
// action_rules.cpp flags its AP costs.
constexpr uint8_t PLACEHOLDER_AP_REGEN_PER_TICK = 2;           // PLACEHOLDER pending BALANCE-001
// PLACEHOLDER pending BALANCE-001. Must stay >= the largest AP cost in
// action_rules.cpp (PAY_GOAL at 10) so every action remains reachable.
constexpr uint8_t PLACEHOLDER_AP_CAP = 10;
constexpr uint8_t PLACEHOLDER_SEEKER_TOKEN_REGEN_PER_TICK = 2; // PLACEHOLDER pending BALANCE-001

// Per-tick resource regeneration (TURN-002):
//  - every hider with active != 0 and status != HIDER_STATUS_CAPTURED gains
//    PLACEHOLDER_AP_REGEN_PER_TICK action points, saturating at PLACEHOLDER_AP_CAP;
//  - the seeker gains PLACEHOLDER_SEEKER_TOKEN_REGEN_PER_TICK connection
//    tokens, saturating at seeker.max_tokens.
// LOA and every other field are left unchanged.
void regenerateForTick(MatchState& state);

} // namespace hider_seeker
