#include "match_end.hpp"
#include "../domain/state.hpp"
#include "rule_types.hpp"

namespace hider_seeker {

void checkMatchEnd(MatchState& match) {
    // If match.phase != PHASE_ACTIVE return immediately
    if (match.phase != PHASE_ACTIVE) {
        return;
    }
    
    // Hider-loss condition: if match.hider_connection_pool == 0 AND 
    // no active hider has server_id != NO_SERVER_ID
    bool hider_loss = (match.hider_connection_pool == 0);
    if (hider_loss) {
        // Check if any active hider has server_id != NO_SERVER_ID
        for (int i = 0; i < MAX_HIDERS; ++i) {
            if (match.hiders[i].active && match.hiders[i].server_id != NO_SERVER_ID) {
                hider_loss = false;
                break;
            }
        }
    }
    
    if (hider_loss) {
        match.phase = PHASE_COMPLETE;
        match.winner = static_cast<uint8_t>(MatchWinner::SEEKER);
        return;
    }
    
    // Seeker-loss condition: evaluated after the hider check
    // If match.seeker.connection_tokens == 0 AND match.seeker_connection_mask == 0
    // and the winner has not already been set to SEEKER
    if (match.seeker.connection_tokens == 0 && match.seeker_connection_mask == 0) {
        match.phase = PHASE_COMPLETE;
        match.winner = static_cast<uint8_t>(MatchWinner::HIDERS);
    }
}

} // namespace hider_seeker