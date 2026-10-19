#include "loa_rules.hpp"
#include "lifecycle_events.hpp"
#include <cstdint>

namespace hider_seeker {

namespace {

// Adds amount to value without exceeding cap. A value already above cap is
// left as it is (regeneration never lowers a resource).
uint8_t saturatingAdd(uint8_t value, uint8_t amount, uint8_t cap) {
    if (value >= cap) {
        return value;
    }
    const uint16_t sum = static_cast<uint16_t>(value) + amount;
    return sum > cap ? cap : static_cast<uint8_t>(sum);
}

// LOA-005: whether each server lifecycle event is public.
// TODO(LOA-005): PLACEHOLDER pending ruleset confirmation — this is an undecided
// design choice listed in the implementation plan's open_questions, not settled
// ruleset canon. Both events are provisionally public; PublicLifecycleEvent
// carries only {type, tick}, so neither discloses which hider owns/occupies the slot.
constexpr bool RULESET_SERVER_ADDED_IS_PUBLIC = true;
constexpr bool RULESET_SERVER_REMOVED_IS_PUBLIC = true;

// Validation-phase check: a public event needs a free slot in the caller's
// output array, so the action can be rejected before any state is modified.
bool hasRoomForLifecycleEvent(bool isDefinedPublicByRuleset,
                              const Array<PublicLifecycleEvent, MAX_EVENTS>& outPublicEvents,
                              uint8_t outPublicEventCount) {
    return !isDefinedPublicByRuleset || outPublicEvents.containsIndex(outPublicEventCount);
}

// Routes the lifecycle event through emitPublicLifecycleEventIfDefined (the LOA-005
// gate), appending the ruleset-layer PublicLifecycleEvent record to the caller's
// output array. It is deliberately NOT written into MatchState::events: folding
// PublicLifecycleEvent into the real domain Event pipeline is a later, explicitly
// out-of-scope job.
void emitLifecycleEvent(const MatchState& match, HiderSeekerEventType type, bool isDefinedPublicByRuleset,
                        Array<PublicLifecycleEvent, MAX_EVENTS>& outPublicEvents,
                        uint8_t& outPublicEventCount) {
    emitPublicLifecycleEventIfDefined(type, match.tick, isDefinedPublicByRuleset, outPublicEvents,
                                      outPublicEventCount);
}

} // namespace

RuleOutcome applyAddServer(MatchState& match, uint8_t hiderId,
                           Array<PublicLifecycleEvent, MAX_EVENTS>& outPublicEvents,
                           uint8_t& outPublicEventCount) {
    if (hiderId >= MAX_HIDERS) {
        return RuleOutcome::REJECTED_INVALID_TARGET;
    }

    // Check if hider has sufficient LOA
    if (match.hiders[hiderId].resources[LOA_RESOURCE_INDEX] < 1) { // Using named constant
        return RuleOutcome::REJECTED_INSUFFICIENT_RESOURCE;
    }

    // Find the lowest-numbered inactive slot
    uint8_t slotId = 0xFF; // Invalid slot
    for (uint8_t i = 0; i < MAX_SERVERS; ++i) {
        if (!match.servers[i].active) {
            slotId = i;
            break;
        }
    }

    // Check if a slot was found
    if (slotId == 0xFF) {
        return RuleOutcome::REJECTED_NO_SLOT_AVAILABLE;
    }

    // The public lifecycle event must fit in the caller's output array.
    if (!hasRoomForLifecycleEvent(RULESET_SERVER_ADDED_IS_PUBLIC, outPublicEvents, outPublicEventCount)) {
        return RuleOutcome::REJECTED_NO_SLOT_AVAILABLE;
    }

    // Deduct 1 LOA from the hider
    match.hiders[hiderId].resources[LOA_RESOURCE_INDEX] -= 1;

    // Activate the slot
    match.servers[slotId].active = 1;
    match.servers[slotId].owner_hider_id = hiderId;
    match.server_count++;

    // Emit public lifecycle event if defined by ruleset (for ADD_SERVER)
    emitLifecycleEvent(match, HiderSeekerEventType::SERVER_ADDED, RULESET_SERVER_ADDED_IS_PUBLIC,
                       outPublicEvents, outPublicEventCount);

    return RuleOutcome::ACCEPTED;
}

RuleOutcome applyRemoveServer(MatchState& match, uint8_t hiderId, uint8_t serverId,
                              Array<PublicLifecycleEvent, MAX_EVENTS>& outPublicEvents,
                              uint8_t& outPublicEventCount) {
    // ---- Validation phase: no state is modified until every check has passed. ----
    if (hiderId >= MAX_HIDERS || serverId >= MAX_SERVERS) {
        return RuleOutcome::REJECTED_INVALID_TARGET;
    }

    // Check if the server is active
    if (!match.servers[serverId].active) {
        return RuleOutcome::REJECTED_INVALID_TARGET;
    }

    // No ownership check: per LOA-002, any hider with sufficient LOA may remove
    // any active slot; rejection depends only on relocation feasibility.

    // Check if hider has sufficient LOA
    if (match.hiders[hiderId].resources[LOA_RESOURCE_INDEX] < 1) { // Using named constant
        return RuleOutcome::REJECTED_INSUFFICIENT_RESOURCE;
    }

    // Occupancy is HiderState::server_id (not server ownership): any active hider
    // whose server_id is the removed slot must relocate in this same action.
    bool hasOccupant = false;
    for (uint8_t i = 0; i < MAX_HIDERS; ++i) {
        if (match.hiders[i].active && match.hiders[i].server_id == serverId) {
            hasOccupant = true;
            break;
        }
    }

    // LOA-002: if occupied, compute the relocation target internally: the
    // lowest-numbered active server slot other than the one being removed.
    uint8_t relocationTargetServerId = NO_SERVER_ID;
    if (hasOccupant) {
        bool targetFound = false;
        for (uint8_t i = 0; i < MAX_SERVERS; ++i) {
            if (match.servers[i].active && i != serverId) {
                relocationTargetServerId = i;
                targetFound = true;
                break;
            }
        }

        // If no other active server exists, reject the whole action
        if (!targetFound) {
            return RuleOutcome::REJECTED_RELOCATION_FAILED;
        }
    }

    // The public lifecycle event must fit in the caller's output array.
    if (!hasRoomForLifecycleEvent(RULESET_SERVER_REMOVED_IS_PUBLIC, outPublicEvents, outPublicEventCount)) {
        return RuleOutcome::REJECTED_NO_SLOT_AVAILABLE;
    }

    // ---- Apply phase: all checks passed, apply every change together. ----

    // Deduct 1 LOA from the hider performing removal
    match.hiders[hiderId].resources[LOA_RESOURCE_INDEX] -= 1;

    // Relocate every occupant to the relocation target
    if (hasOccupant) {
        for (uint8_t i = 0; i < MAX_HIDERS; ++i) {
            if (match.hiders[i].active && match.hiders[i].server_id == serverId) {
                match.hiders[i].server_id = relocationTargetServerId;
            }
        }
    }

    // LOA-004: cancel/remove/reset every reference to the removed slot (see loa_rules.hpp)
    invalidateServerReferences(match, serverId);

    // Reset (clears metrics/history/ownership/traces and deactivates) the old slot
    resetServerSlot(match.servers[serverId]);
    match.server_count--;

    // Emit public lifecycle event if defined by ruleset (for REMOVE_SERVER)
    emitLifecycleEvent(match, HiderSeekerEventType::SERVER_REMOVED, RULESET_SERVER_REMOVED_IS_PUBLIC,
                       outPublicEvents, outPublicEventCount);

    return RuleOutcome::ACCEPTED;
}

void resetServerSlot(ServerState& server) {
    // Clear metrics
    server.current = {};
    for (uint8_t i = 0; i < HISTORY_TICKS; ++i) {
        server.history[i] = {};
    }
    server.history_head = 0;

    // Clear ownership
    server.owner_hider_id = 0xFF;
    server.active = 0;

    // Clear trace fields
    server.trace_strength = 0;
    server.trace_age = 0;

    // Clear any public flags
    server.public_flags = 0;
}

void invalidateServerReferences(MatchState& match, uint8_t serverId) {
    // LOA-004 rule; documented in loa_rules.hpp.
    // TODO(LOA-004): PLACEHOLDER cancel policy pending ruleset confirmation.
    if (serverId >= MAX_SERVERS) {
        return;
    }

    // Goals: cancel any active goal with a server-referencing requirement on this slot.
    for (uint8_t g = 0; g < MAX_GOALS; ++g) {
        GoalState& goal = match.goals[g];
        if (!goal.active) continue;

        const uint8_t count = goal.requirement_count < MAX_REQUIREMENTS_PER_GOAL
                            ? goal.requirement_count : MAX_REQUIREMENTS_PER_GOAL;
        bool referencesRemovedSlot = false;
        for (uint8_t r = 0; r < count; ++r) {
            const Requirement& req = goal.requirements[r];
            const bool isServerRequirement = req.type == REQ_SERVER_METRIC_THRESHOLD
                                          || req.type == REQ_CONFIGURED_SERVER;
            if (isServerRequirement && req.subject_id == serverId) {
                referencesRemovedSlot = true;
                break;
            }
        }

        if (referencesRemovedSlot) {
            goal.active = 0;
            goal.status = GOAL_STATUS_CANCELLED_SERVER_REMOVED;
            for (uint8_t h = 0; h < MAX_HIDERS; ++h) {
                match.hiders[h].goal_progress[g] = 0;
            }
        }
    }

    // Pending seeker targets: drop entries for this slot, compacting the
    // survivors in their original order starting at target_head.
    SeekerState& seeker = match.seeker;
    const uint8_t liveCount = seeker.target_count < MAX_SEEKER_TARGETS
                            ? seeker.target_count : MAX_SEEKER_TARGETS;
    Array<uint8_t, MAX_SEEKER_TARGETS> kept;
    uint8_t keptCount = 0;
    for (uint8_t i = 0; i < liveCount; ++i) {
        const uint8_t id = seeker.target_ids[(seeker.target_head + i) % MAX_SEEKER_TARGETS];
        if (id != serverId) {
            kept.set(keptCount, id);
            keptCount++;
        }
    }
    for (uint8_t i = 0; i < MAX_SEEKER_TARGETS; ++i) {
        const uint8_t index = static_cast<uint8_t>((seeker.target_head + i) % MAX_SEEKER_TARGETS);
        seeker.target_ids[index] = i < keptCount ? kept.getOrDefault(i, 0) : 0;
    }
    seeker.target_count = keptCount;

    // Seeker per-slot knowledge is reset.
    seeker.belief[serverId] = 0;
    seeker.last_probe_tick[serverId] = 0;
    
    // CONN-007: If the seeker was connected to this server, refund a connection token
    const uint16_t maskBit = 1u << serverId;
    if (match.seeker_connection_mask & maskBit) {
        match.seeker_connection_mask &= ~maskBit;  // Clear the bit
        match.seeker.connection_tokens = saturatingAdd(match.seeker.connection_tokens, 1, 255);  // Increment with saturating addition at 255
    }
}

} // namespace hider_seeker