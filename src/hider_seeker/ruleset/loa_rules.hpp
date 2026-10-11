#pragma once

#include "rule_types.hpp"
#include "../domain/state.hpp"

namespace hider_seeker {

RuleOutcome applyAddServer(MatchState&, uint8_t hiderId);
RuleOutcome applyRemoveServer(MatchState&, uint8_t hiderId, uint8_t serverId);
void resetServerSlot(ServerState&);
void invalidateServerReferences(MatchState&, uint8_t serverId);

} // namespace hider_seeker