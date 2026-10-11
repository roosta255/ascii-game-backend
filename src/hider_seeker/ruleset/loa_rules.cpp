#include "loa_rules.hpp"
#include "lifecycle_events.hpp"
#include <cstdint>

namespace hider_seeker {

RuleOutcome applyAddServer(MatchState& match, uint8_t hiderId) {
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
    emitPublicLifecycleEventIfDefined(HiderSeekerEventType::SERVER_ADDED, match.tick, true, match.events);

    return RuleOutcome::ACCEPTED;
}

RuleOutcome applyRemoveServer(MatchState& match, uint8_t hiderId, uint8_t serverId) {
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

    // Check if there's an occupant that needs relocation
    uint8_t occupantHiderId = match.servers[serverId].owner_hider_id;
    
    // If the server has no occupant, proceed with removal
    if (occupantHiderId == 0xFF) {
        // Deduct 1 LOA from the hider
        match.hiders[hiderId].resources[0] -= 1;

        // Reset the server slot
        resetServerSlot(match.servers[serverId]);

        // Deactivate the slot
        match.servers[serverId].active = 0;
        match.server_count--;

        // Emit public lifecycle event if defined by ruleset (for REMOVE_SERVER)
        emitPublicLifecycleEventIfDefined(HiderSeekerEventType::SERVER_REMOVED, match.tick, true, match.events);

        return RuleOutcome::ACCEPTED;
    }

    // Find a new slot for the occupant hider (lowest-numbered inactive slot)
    uint8_t newSlotId = 0xFF; // Invalid slot
    for (uint8_t i = 0; i < MAX_SERVERS; ++i) {
        if (!match.servers[i].active) {
            newSlotId = i;
            break;
        }
    }

    // If no slot is available, reject the whole operation
    if (newSlotId == 0xFF) {
        return RuleOutcome::REJECTED_NO_SLOT_AVAILABLE;
    }

    // Relocate the occupant to the new slot
    match.servers[newSlotId].active = 1;
    match.servers[newSlotId].owner_hider_id = occupantHiderId;
    match.hiders[occupantHiderId].server_id = newSlotId;

    // Deduct 1 LOA from the hider performing removal
    match.hiders[hiderId].resources[0] -= 1;

    // Reset the old server slot
    resetServerSlot(match.servers[serverId]);

    // Deactivate the old slot
    match.servers[serverId].active = 0;
    match.server_count--;

    // Invalidate references to the removed server
    invalidateServerReferences(match, serverId);

    // Emit public lifecycle event if defined by ruleset (for REMOVE_SERVER)
    emitPublicLifecycleEventIfDefined(HiderSeekerEventType::SERVER_REMOVED, match.tick, true, match.events);

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
    // Policy: When a server is removed, we cancel any pending hider intents 
    // that reference this server and reset seeker targets that point to it
    
    // Cancel any pending hider intents referencing the removed server
    for (uint8_t hiderIdx = 0; hiderIdx < MAX_HIDERS; ++hiderIdx) {
        if (match.hiders[hiderIdx].active && match.hiders[hiderIdx].server_id == serverId) {
            // Clear the hider's server_id to indicate it's no longer occupying a server
            match.hiders[hiderIdx].server_id = 0xFF;
        }
    }

    // Reset seeker targets that reference this server
    for (uint8_t i = 0; i < MAX_SEEKER_TARGETS; ++i) {
        if (match.seeker.target_ids[i] == serverId) {
            match.seeker.target_ids[i] = 0xFF;
        }
    }

    // Reset seeker belief entries for this server
    match.seeker.belief[serverId] = 0;

    // Reset last probe tick entries for this server
    match.seeker.last_probe_tick[serverId] = 0;
    
    // Cancel any goals that reference this server (if they exist)
    for (uint8_t goalIdx = 0; goalIdx < MAX_GOALS; ++goalIdx) {
        if (match.goals[goalIdx].active) {
            for (uint8_t reqIdx = 0; reqIdx < match.goals[goalIdx].requirement_count; ++reqIdx) {
                if (match.goals[goalIdx].requirements[reqIdx].subject_id == serverId) {
                    // For now, we'll just mark the goal as inactive to cancel it
                    // A more sophisticated approach would be to re-evaluate the goal's status
                    match.goals[goalIdx].active = 0;
                }
            }
        }
    }
}

} // namespace hider_seeker