#include "loa_rules.hpp"
#include "lifecycle_events.hpp"
#include <cstdint>

namespace hider_seeker {

namespace {

// Ruleset decision (LOA-005): whether each server lifecycle event is public.
// Both are public under the initial ruleset; PublicLifecycleEvent carries only
// {type, tick}, so neither event discloses which hider owns/occupies the slot.
constexpr bool RULESET_SERVER_ADDED_IS_PUBLIC = true;
constexpr bool RULESET_SERVER_REMOVED_IS_PUBLIC = true;

// Marker used to detect whether emitPublicLifecycleEventIfDefined appended anything.
constexpr HiderSeekerEventType NOT_EMITTED = static_cast<HiderSeekerEventType>(0xFF);

void appendMatchEvent(MatchState& match, const PublicLifecycleEvent& event) {
    // match.events is a ring buffer (event_head = oldest, event_count = live entries).
    // When full, the oldest entry is overwritten.
    uint8_t index = static_cast<uint8_t>((match.event_head + match.event_count) % MAX_EVENTS);
    match.events[index].type = static_cast<uint8_t>(event.type);
    match.events[index].tick = event.tick;
    if (match.event_count < MAX_EVENTS) {
        match.event_count++;
    } else {
        match.event_head = static_cast<uint8_t>((match.event_head + 1) % MAX_EVENTS);
    }
}

// Routes the lifecycle event through emitPublicLifecycleEventIfDefined (the LOA-005
// gate) and records whatever it emitted into the match's public event log.
void emitLifecycleEvent(MatchState& match, HiderSeekerEventType type, bool isDefinedPublicByRuleset) {
    Array<PublicLifecycleEvent, MAX_EVENTS> emitted;
    emitted.assignValue(0, PublicLifecycleEvent{NOT_EMITTED, 0});
    emitPublicLifecycleEventIfDefined(type, match.tick, isDefinedPublicByRuleset, emitted);
    emitted.access(0, [&match](PublicLifecycleEvent& event) {
        if (event.type != NOT_EMITTED) {
            appendMatchEvent(match, event);
        }
    });
}

} // namespace

RuleOutcome applyAddServer(MatchState& match, uint8_t hiderId) {
    if (hiderId >= MAX_HIDERS) {
        return RuleOutcome::REJECTED_INVALID_TARGET;
    }

    // Check if hider has sufficient LOA
    if (match.hiders[hiderId].resources[0] < 1) { // Assuming resource type 0 is LOA
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
    match.hiders[hiderId].resources[0] -= 1;

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
    if (match.hiders[hiderId].resources[0] < 1) { // Assuming resource type 0 is LOA
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
    match.hiders[hiderId].resources[0] -= 1;

    // Relocate every occupant to the relocation target
    if (hasOccupant) {
        for (uint8_t i = 0; i < MAX_HIDERS; ++i) {
            if (match.hiders[i].active && match.hiders[i].server_id == serverId) {
                match.hiders[i].server_id = relocationTargetServerId;
            }
        }
    }

    // LOA-004: resolve goals / seeker targets / per-slot seeker state referencing the slot
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
    // Ruleset policy (LOA-004): SCAFFOLDED. This function is a placeholder
    // implementation that does not yet implement the final ruleset decision.
    // LOA-004 is an open question and must be resolved through the proper
    // ruleset decision process before this behavior can be considered final.
    // For now, this function is a no-op to avoid shipping uncommitted behavior.
    // TODO: Implement the actual LOA-004 policy once it's decided in the ruleset.
    (void)match;
    (void)serverId;
}

} // namespace hider_seeker