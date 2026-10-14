#pragma once

#include "../domain/state.hpp"
#include "../ruleset/target_ranking_interface.hpp"
#include "turn_types.hpp"

namespace hider_seeker {

// Common entry checks (TURN-008), applied by all four entry points:
//  - state.phase != PHASE_ACTIVE      -> REJECTED / MATCH_NOT_ACTIVE
//  - hider id out of range            -> REJECTED / RULE_REJECTED (REJECTED_INVALID_TARGET)
//  - hider captured                   -> REJECTED / HIDER_CAPTURED
//  - hider inactive                   -> REJECTED / RULE_REJECTED (REJECTED_INVALID_TARGET)
// Every REJECTED result leaves state completely unchanged; every other result
// increases state.revision by exactly 1.
//
// playMove: applies the move now if affordable (no tick, no seeker policy
// call); otherwise advances ticks until it applies if regeneration makes it
// affordable within MAX_ADVANCE_TICKS ticks, else REJECTED / NEVER_AFFORDABLE.
// queueMove: applies the move now if affordable; otherwise appends it to the
// pending ring (QUEUED) without advancing any tick.
// passTicks: advances exactly `ticks` (1..MAX_ADVANCE_TICKS) ticks.
// cancelPendingMove: removes hiderId's own pending move moveId, or NOT_FOUND.

TurnResult playMove(MatchState& state, const MoveRequest& move, iTargetRankingPolicy& seekerPolicy);
TurnResult queueMove(MatchState& state, const MoveRequest& move, iTargetRankingPolicy& seekerPolicy);
TurnResult passTicks(MatchState& state, uint8_t ticks, iTargetRankingPolicy& seekerPolicy);
TurnResult cancelPendingMove(MatchState& state, uint8_t hiderId, uint32_t moveId);

} // namespace hider_seeker