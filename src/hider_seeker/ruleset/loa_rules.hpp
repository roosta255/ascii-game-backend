#pragma once

#include "rule_types.hpp"
#include "../domain/state.hpp"

namespace hider_seeker {

// Sentinel meaning "no relocation target supplied" / "hider occupies no server".
constexpr uint8_t NO_SERVER_ID = 0xFF;

RuleOutcome applyAddServer(MatchState&, uint8_t hiderId);
// REMOVE_SERVER carries the slot being removed plus the relocation target for any
// hider occupying it (design §4.2 / §API). If the removed slot is occupied and the
// relocation target is not a valid, active, different slot, the whole action is
// rejected with no state change (LOA-002).
RuleOutcome applyRemoveServer(MatchState&, uint8_t hiderId, uint8_t serverId,
                              uint8_t relocationTargetServerId = NO_SERVER_ID);
void resetServerSlot(ServerState&);
void invalidateServerReferences(MatchState&, uint8_t serverId);

} // namespace hider_seeker