#include "match_end.hpp"
#include "../domain/state.hpp"
#include "goal_rules.hpp"

namespace hider_seeker {

void checkMatchEnd(MatchState& match) {
    // Do nothing unless the match is active
    if (match.phase != PHASE_ACTIVE) {
        return;
    }
    
    // The hiders win when their completed goals' rewards reach the target
    // (checked first: a win in the same tick as a loss condition goes to the hiders).
    if (teamReward(match) >= PLACEHOLDER_HIDER_REWARD_TARGET) {
        match.phase = PHASE_COMPLETE;
        match.winner = static_cast<uint8_t>(MatchWinner::HIDERS);
        return;
    }
    
    // Hiders lose: hider_connection_pool == 0 and no active hider has server_id != NO_SERVER_ID
    bool hiders_lose = (match.hider_connection_pool == 0);
    if (hiders_lose) {
        // Check if any active hider is connected to a server
        for (int i = 0; i < MAX_HIDERS; ++i) {
            if (match.hiders[i].active && match.hiders[i].server_id != NO_SERVER_ID) {
                hiders_lose = false;
                break;
            }
        }
    }
    
    if (hiders_lose) {
        match.phase = PHASE_COMPLETE;
        match.winner = static_cast<uint8_t>(MatchWinner::SEEKER);
        return;
    }
    
    // Seeker loses: connection_tokens == 0 and seeker_connection_mask == 0
    if (match.seeker.connection_tokens == 0 && match.seeker_connection_mask == 0) {
        match.phase = PHASE_COMPLETE;
        match.winner = static_cast<uint8_t>(MatchWinner::HIDERS);
    }
}

} // namespace hider_seeker