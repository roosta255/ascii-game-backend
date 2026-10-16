#include "connection_rules.hpp"
#include "../ruleset/action_rules.hpp"
#include "../domain/state.hpp"
#include <cstdint>

namespace hider_seeker {

RuleOutcome applyConnect(MatchState& state, uint8_t hiderId, uint8_t serverId) {
    // CONN-002: Validation order
    // 1. Check if hider is valid and active
    if (hiderId >= MAX_HIDERS || !state.hiders[hiderId].active) {
        return RuleOutcome::REJECTED_INVALID_TARGET;
    }
    
    // 2. Check if server is valid and active
    if (serverId >= MAX_SERVERS || !state.servers[serverId].active) {
        return RuleOutcome::REJECTED_INVALID_TARGET;
    }
    
    // 3. Check if hider is already connected
    if (state.hiders[hiderId].server_id != NO_SERVER_ID) {
        return RuleOutcome::REJECTED_INVALID_TARGET;
    }
    
    // 4. Check if connection pool is empty
    if (state.hider_connection_pool == 0) {
        return RuleOutcome::REJECTED_NO_CONNECTIONS;
    }
    
    // 5. Check if hider has sufficient AP
    const uint16_t apCost = 0; // No cost for connect action
    if (state.hiders[hiderId].resources.ap < apCost) {
        return RuleOutcome::REJECTED_INSUFFICIENT_RESOURCE;
    }
    
    // 6. Apply the connection
    state.hiders[hiderId].resources.ap -= apCost;
    state.hider_connection_pool -= 1;
    state.hiders[hiderId].server_id = serverId;
    
    return RuleOutcome::ACCEPTED;
}

RuleOutcome applyDisconnect(MatchState& state, uint8_t hiderId) {
    // CONN-003: Validation order
    // 1. Check if hider is valid and active
    if (hiderId >= MAX_HIDERS || !state.hiders[hiderId].active) {
        return RuleOutcome::REJECTED_INVALID_TARGET;
    }
    
    // 2. Check if hider is connected
    if (state.hiders[hiderId].server_id == NO_SERVER_ID) {
        return RuleOutcome::REJECTED_INVALID_TARGET;
    }
    
    // 3. Check if hider has sufficient AP
    const uint16_t apCost = 0; // No cost for disconnect action
    if (state.hiders[hiderId].resources.ap < apCost) {
        return RuleOutcome::REJECTED_INSUFFICIENT_RESOURCE;
    }
    
    // 4. Apply the disconnection
    state.hiders[hiderId].resources.ap -= apCost;
    state.hider_connection_pool += 1; // saturating at 255 (implicit in uint8_t)
    if (state.hider_connection_pool > 255) {
        state.hider_connection_pool = 255;
    }
    state.hiders[hiderId].server_id = NO_SERVER_ID;
    
    return RuleOutcome::ACCEPTED;
}

} // namespace hider_seeker