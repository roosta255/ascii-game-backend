#pragma once

#include "../hider_seeker/domain/state.hpp"
#include "../hider_seeker/ruleset/target_ranking_interface.hpp"
#include "../hider_seeker/turn/turn_types.hpp"

namespace hider_seeker {

TurnResult playMove(MatchState& state, const MoveRequest& move, iTargetRankingPolicy& seekerPolicy);
TurnResult queueMove(MatchState& state, const MoveRequest& move, iTargetRankingPolicy& seekerPolicy);
TurnResult passTicks(MatchState& state, uint8_t ticks, iTargetRankingPolicy& seekerPolicy);
TurnResult cancelPendingMove(MatchState& state, uint8_t hiderId, uint32_t moveId);

} // namespace hider_seeker