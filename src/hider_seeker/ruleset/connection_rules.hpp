#pragma once

#include "rule_types.hpp"
#include "../domain/state.hpp"
#include "../tick/tick_types.hpp"

namespace hider_seeker {

RuleOutcome applyConnect(MatchState& state, uint8_t hiderId, uint8_t serverId);
RuleOutcome applyDisconnect(MatchState& state, uint8_t hiderId);
void applyWager(MatchState& match, uint8_t serverId, uint8_t wager, TickOutput& output);

} // namespace hider_seeker