#pragma once

#include "rule_types.hpp"
#include "../domain/state.hpp"

namespace hider_seeker {

// LOA is stored in HiderState::resources at this index. The frozen domain layer
// does not name resource indices, so this ruleset constant is the single place
// that binds "LOA" to a resources[] slot.
constexpr uint8_t LOA_RESOURCE_INDEX = 0;

// Ruleset-defined GoalState::status value for a goal cancelled by LOA-004
// because one of its requirements referenced a removed server slot.
constexpr uint8_t GOAL_STATUS_CANCELLED_SERVER_REMOVED = 0xFE;

RuleOutcome applyAddServer(MatchState&, uint8_t hiderId);
// REMOVE_SERVER carries the slot being removed plus the relocation target for any
// hider occupying it (design §4.2 / §API). If the removed slot is occupied and the
// relocation target is not a valid, active, different slot, the whole action is
// rejected with no state change (LOA-002).
RuleOutcome applyRemoveServer(MatchState&, uint8_t hiderId, uint8_t serverId,
                              uint8_t relocationTargetServerId = NO_SERVER_ID);
void resetServerSlot(ServerState&);
// LOA-004: called by applyRemoveServer inside the same committed operation.
// Explicit rule for every reference to the removed slot:
//  - GoalState: any active goal with a server-referencing requirement
//    (REQ_SERVER_METRIC_THRESHOLD / REQ_CONFIGURED_SERVER) whose subject_id is
//    the removed slot is CANCELLED: active = 0, status =
//    GOAL_STATUS_CANCELLED_SERVER_REMOVED, and every hider's goal_progress for
//    it is reset to 0. Requirements of other types (whose subject_id is not a
//    server id) are left untouched.
//  - Pending seeker targets: every entry equal to the removed slot is REMOVED
//    from the ring buffer; the remaining entries keep their relative order and
//    are compacted starting at target_head.
//  - Seeker belief / last_probe_tick for the slot are reset to 0, so a later
//    ADD_SERVER reusing the slot starts with no stale seeker knowledge.
//  - Pending intents: MatchState stores no pending intents (IntentRecord
//    carries no server id), so they are INVALIDATED by revalidation at apply
//    time: a later REMOVE_SERVER naming the slot is rejected
//    (REJECTED_INVALID_TARGET) because the slot is inactive, and metric effects
//    target the occupant's (already relocated) server_id.
void invalidateServerReferences(MatchState&, uint8_t serverId);

} // namespace hider_seeker