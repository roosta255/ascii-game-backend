#include "loa_rules.hpp"
#include "lifecycle_events.hpp"
#include <cstdint>

namespace hider_seeker {

namespace {

// Ruleset decision (LOA-004): goal cancel policy.
// TODO: This is a placeholder for an undecided design choice (LOA-004).
// See implementation plan's open_questions list.
constexpr uint8_t GOAL_STATUS_CANCELLED_SERVER_REMOVED = 0xFE;

// Ruleset decision (LOA-005): whether each server lifecycle event is public.
// Both are public under the initial ruleset; PublicLifecycleEvent carries only
// {type, tick}, so neither event discloses which hider owns/occupies the slot.
// TODO: This is a placeholder for an undecided design choice (LOA-005).
// See implementation plan's open_questions list.
constexpr bool RULESET_SERVER_ADDED_IS_PUBLIC = true;
constexpr bool RULESET_SERVER_REMOVED_IS_PUBLIC = true;

// Routes the lifecycle event through emitPublicLifecycleEventIfDefined (the LOA-005
// gate) and records whatever it emitted into the match's public event log.
void emitLifecycleEvent(MatchState& match, HiderSeekerEventType type, bool isDefinedPublicByRuleset) {
    Array<PublicLifecycleEvent, MAX_EVENTS> emitted;
    uint8_t emittedCount = 0;
    emitPublicLifecycleEventIfDefined(type, match.tick, isDefinedPublicByRuleset, emitted, emittedCount);
    // TODO: The implementation plan explicitly states that folding PublicLifecycleEvent
    // into the real Event/event pipeline is an out-of-scope job. This code should
    // only produce the ruleset-layer record described in the plan.
}

} // namespace

RuleOutcome applyAddServer(MatchState& match, uint8_t hiderId) {
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

    // Deduct 1 LOA from the hider
    match.hiders[hiderId].resources[LOA_RESOURCE_INDEX] -= 1;

    // Activate the slot
    match.servers[slotId].active = 1;
    match.servers[slotId].owner_hider_id = hiderId;
    match.server_count++;

    // Emit public lifecycle event if defined by ruleset (for ADD_SERVER)
    emitLifecycleEvent(match, HiderSeekerEventType::SERVER_ADDED, RULESET_SERVER_ADDED_IS_PUBLIC);

    return RuleOutcome::ACCEPTED;
}

RuleOutcome applyRemoveServer(MatchState& match, uint8_t hiderId, uint8_t serverId,
                              uint8_t relocationTargetServerId) {
    // ---- Validation phase: no state is modified until every check has passed. ----
    if (hiderId >= MAX_HIDERS || serverId >= MAX_SERVERS) {
        return RuleOutcome::REJECTED_INVALID_TARGET;
    }

    // Check if the server is active
    if (!match.servers[serverId].active) {
        return RuleOutcome::REJECTED_INVALID_TARGET;
    }

    // Check if the hider owns this server
    if (match.servers[serverId].owner_hider_id != hiderId) {
        return RuleOutcome::REJECTED_INVALID_TARGET;
    }

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

    // LOA-002: if occupied, the supplied relocation target must be a valid, active
    // slot other than the one being removed; otherwise reject the whole action.
    if (hasOccupant) {
        bool targetValid = relocationTargetServerId < MAX_SERVERS
                        && relocationTargetServerId != serverId
                        && match.servers[relocationTargetServerId].active;
        if (!targetValid) {
            return RuleOutcome::REJECTED_RELOCATION_FAILED;
        }
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
    emitLifecycleEvent(match, HiderSeekerEventType::SERVER_REMOVED, RULESET_SERVER_REMOVED_IS_PUBLIC);

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
    // Explicit LOA-004 rule; documented in loa_rules.hpp.
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
}

} // namespace hider_seeker