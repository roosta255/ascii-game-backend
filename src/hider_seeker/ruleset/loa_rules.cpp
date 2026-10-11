#include "loa_rules.hpp"
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

    // TODO: Implement occupant relocation logic here
    // For now, we'll just reject if relocation would fail
    // This is a placeholder for the actual implementation
    
    // Deduct 1 LOA from the hider
    match.hiders[hiderId].resources[0] -= 1;

    // Reset the server slot (this will be called after relocation)
    resetServerSlot(match.servers[serverId]);

    // Deactivate the slot
    match.servers[serverId].active = 0;
    match.server_count--;

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
    // This is a placeholder function for the undecided policy (LOA-004)
    // The actual implementation will be determined by the open questions
    
    // For now, we'll implement a no-op as a scaffold
    // The real implementation should walk:
    // 1. Every GoalState/Requirement referencing the removed slot
    // 2. Every still-pending hider intent reference 
    // 3. The seeker's target_ids/belief/last_probe_tick entries for that slot
    // And apply one of the explicit rules: cancel, rebind, or invalidate
    
    // This function is intentionally left unimplemented as per the plan
    // to avoid guessing the policy (LOA-004)
}

} // namespace hider_seeker