#pragma once

#include "rule_types.hpp"
#include "lifecycle_events.hpp"
#include "../domain/state.hpp"

namespace hider_seeker {

// LOA is stored in HiderState::resources at this index. The frozen domain layer
// does not name resource indices, so this ruleset constant is the single place
// that binds "LOA" to a resources[] slot.
constexpr uint8_t LOA_RESOURCE_INDEX = 0;

// Ruleset-defined GoalState::status value for a goal cancelled by LOA-004
// because one of its requirements referenced a removed server slot.
// TODO(LOA-004): PLACEHOLDER pending ruleset confirmation — both the goal cancel
// policy below and this status value are undecided design choices listed in the
// implementation plan's open_questions, not settled ruleset canon.
constexpr uint8_t GOAL_STATUS_CANCELLED_SERVER_REMOVED = 0xFE;

// LOA-005 output channel: on ACCEPTED, the server lifecycle event is appended
// at outPublicEvents[outPublicEventCount] (incrementing the count) when the
// ruleset defines it as public — see emitPublicLifecycleEventIfDefined. Callers
// may batch several actions into the same array by reusing the same count. On
// any rejection nothing is appended. If the event is public and the array is
// already full, the whole action is rejected (REJECTED_NO_SLOT_AVAILABLE) with
// no state change, so an accepted action never silently drops its event.
RuleOutcome applyAddServer(MatchState&, uint8_t hiderId,
                           Array<PublicLifecycleEvent, MAX_EVENTS>& outPublicEvents,
                           uint8_t& outPublicEventCount);
// REMOVE_SERVER carries only the slot being removed. If the removed slot is
// occupied, the relocation target is computed internally as part of the same
// atomic operation (LOA-002): the lowest-numbered ACTIVE server slot other than
// the removed one. If no such slot exists, the whole action is rejected
// (REJECTED_RELOCATION_FAILED) with no state change.
RuleOutcome applyRemoveServer(MatchState&, uint8_t hiderId, uint8_t serverId,
                              Array<PublicLifecycleEvent, MAX_EVENTS>& outPublicEvents,
                              uint8_t& outPublicEventCount);
void resetServerSlot(ServerState&);
// LOA-004: called by applyRemoveServer inside the same committed operation.
// Provisional rule (TODO(LOA-004): placeholder pending ruleset confirmation)
// for every reference to the removed slot:
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