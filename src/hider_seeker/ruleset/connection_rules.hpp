#pragma once

#include "rule_types.hpp"
#include "../domain/state.hpp"

namespace hider_seeker {

RuleOutcome applyConnect(MatchState& state, uint8_t hiderId, uint8_t serverId);
RuleOutcome applyDisconnect(MatchState& state, uint8_t hiderId);

} // namespace hider_seeker