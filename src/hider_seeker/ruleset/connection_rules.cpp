#include "connection_rules.hpp"
#include "../ruleset/action_rules.hpp"
#include "../domain/state.hpp"
#include "../tick/tick_types.hpp"
#include <cstdint>
#include <algorithm>

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
    if (state.hiders[hiderId].action_points < apCost) {
        return RuleOutcome::REJECTED_INSUFFICIENT_RESOURCE;
    }
    
    // 6. Apply the connection
    state.hiders[hiderId].action_points -= apCost;
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
    if (state.hiders[hiderId].action_points < apCost) {
        return RuleOutcome::REJECTED_INSUFFICIENT_RESOURCE;
    }
    
    // 4. Apply the disconnection
    state.hiders[hiderId].action_points -= apCost;
    state.hider_connection_pool += 1; // saturating at 255 (implicit in uint8_t)
    if (state.hider_connection_pool > 255) {
        state.hider_connection_pool = 255;
    }
    state.hiders[hiderId].server_id = NO_SERVER_ID;
    
    return RuleOutcome::ACCEPTED;
}

void applyWager(MatchState& match, uint8_t serverId, uint8_t wager, TickOutput& output) {
    // Cap the wager to available connection tokens
    uint8_t capped_wager = std::min(wager, match.seeker.connection_tokens);
    
    // If capped_wager is 0, return immediately (no state change, no events)
    if (capped_wager == 0) {
        return;
    }
    
    // Compute ordered set H of active hiders whose server_id == serverId
    uint8_t hider_indices[16]; // MAX_HIDERS is 16
    int hider_count = 0;
    
    for (int i = 0; i < MAX_HIDERS; ++i) {
        if (match.hiders[i].active && match.hiders[i].server_id == serverId) {
            hider_indices[hider_count++] = i;
        }
    }
    
    // destroyed = min(capped_wager, |H|)
    uint8_t destroyed = std::min(capped_wager, static_cast<uint8_t>(hider_count));
    
    // For the first destroyed hiders in H, set server_id = NO_SERVER_ID
    for (int i = 0; i < destroyed; ++i) {
        match.hiders[hider_indices[i]].server_id = NO_SERVER_ID;
    }
    
    // lost = capped_wager - destroyed
    uint8_t lost = capped_wager - destroyed;
    
    // Decrement match.seeker.connection_tokens by lost
    match.seeker.connection_tokens -= lost;
    
    // Set match.seeker.belief[serverId] = destroyed
    match.seeker.belief[serverId] = destroyed;
    
    // Set match.seeker.last_probe_tick[serverId] = static_cast<uint8_t>(match.tick & 0xFF)
    match.seeker.last_probe_tick[serverId] = static_cast<uint8_t>(match.tick & 0xFF);
    
    // If output.publicEvents.containsIndex(output.publicEventCount) is true, append PublicLifecycleEvent
    if (output.publicEvents.containsIndex(output.publicEventCount)) {
        uint8_t outcome = (destroyed > 0) ? PROBE_OUTCOME_HIT : PROBE_OUTCOME_MISS;
        PublicLifecycleEvent event{HiderSeekerEventType::SERVER_PROBED, match.tick, serverId, outcome};
        output.publicEvents.assignValue(output.publicEventCount, event);
        output.publicEventCount++;
    }
    
    // If output.wagers.containsIndex(output.wagerCount) is true, append WagerRecord
    if (output.wagers.containsIndex(output.wagerCount)) {
        WagerRecord wager_record{serverId, capped_wager, destroyed, lost};
        output.wagers.assignValue(output.wagerCount, wager_record);
        output.wagerCount++;
    }
    
    // Do not modify match.seeker.seeker_connection_mask
}

} // namespace hider_seeker