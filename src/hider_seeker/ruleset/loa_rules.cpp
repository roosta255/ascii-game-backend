#include "loa_rules.hpp"
#include "lifecycle_events.hpp"
#include <cstdint>

namespace hider_seeker {

// LOA resource index - assuming resource type 0 is LOA
constexpr uint8_t LOA_RESOURCE_INDEX = 0;

namespace {

// Ruleset decision (LOA-005): whether each server lifecycle event is public.
// Both are public under the initial ruleset; PublicLifecycleEvent carries only
// {type, tick}, so neither event discloses which hider owns/occupies the slot.
constexpr bool RULESET_SERVER_ADDED_IS_PUBLIC = true;
constexpr bool RULESET_SERVER_REMOVED_IS_PUBLIC = true;

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
    uint8_t emittedCount = 0;
    emitPublicLifecycleEventIfDefined(type, match.tick, isDefinedPublicByRuleset, emitted, emittedCount);
    for (int i = 0; i < emittedCount; ++i) {
        emitted.access(i, [&match](PublicLifecycleEvent& event) {
            appendMatchEvent(match, event);
        });
    }
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

    // LOA-004 hook (scaffold; policy undecided): references to the removed slot
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
    // LOA-004: Implement the required behavior for references to removed slot
    //
    // For each goal requirement that references the removed server:
    // - If it's a server metric threshold or configured server requirement,
    //   cancel the requirement (set to inactive)
    // For pending seeker targets:
    // - Remove any target that references the removed server
    // For seeker state:
    // - Clear belief for the removed server
    // - Clear last_probe_tick for the removed server
    
    // Handle goal requirements referencing the removed server
    for (uint8_t g = 0; g < MAX_GOALS; ++g) {
        if (!match.goals[g].active) continue;
        
        for (uint8_t r = 0; r < match.goals[g].requirement_count; ++r) {
            Requirement& req = match.goals[g].requirements[r];
            
            // If this requirement references the removed server
            if (req.subject_id == serverId) {
                // Cancel requirements that reference the removed server
                // This is a simple approach - in a real implementation, 
                // we might want to handle different requirement types differently
                req.type = REQ_CUSTOM_RULE; // Mark as invalid/cancelled
            }
        }
    }
    
    // Handle pending seeker targets referencing the removed server
    for (uint8_t i = 0; i < match.seeker.target_count; ++i) {
        uint8_t index = (match.seeker.target_head + i) % MAX_SEEKER_TARGETS;
        if (match.seeker.target_ids[index] == serverId) {
            // Remove this target by decrementing count and adjusting head
            // This is a simple removal approach - in practice we might want to 
            // shift elements or use a more sophisticated eviction policy
            match.seeker.target_count--;
            if (match.seeker.target_count > 0) {
                // Adjust the head to maintain correct order
                match.seeker.target_head = (match.seeker.target_head + 1) % MAX_SEEKER_TARGETS;
            }
        }
    }
    
    // Handle seeker belief and last_probe_tick for the removed server
    match.seeker.belief[serverId] = 0; // Clear belief
    match.seeker.last_probe_tick[serverId] = 0; // Clear last probe tick
}

} // namespace hider_seeker