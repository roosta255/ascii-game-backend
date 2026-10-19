#include "seeker_turn.hpp"
#include "../ruleset/connection_rules.hpp"
#include "feature_extract.hpp"

namespace hider_seeker {

void runSeekerTurn(MatchState& state, const iSeekerPolicy& policy, TickOutput& output) {
    // Phase guard: if not active, return immediately
    if (state.phase != PHASE_ACTIVE) {
        return;
    }

    // Score phase
    SeekerObservation obs = buildSeekerObservation(state);
    Array<SeekerServerScores, MAX_SERVERS> scores{};
    
    policy.score(obs, scores);

    // Wager phase
    for (int s = 0; s < MAX_SERVERS; ++s) {
        if ((state.seeker_connection_mask & (1u << s)) && state.servers[s].active) {
            const SeekerServerScores& score = scores.getOrDefault(s, SeekerServerScores{0, 0, 0});
            // Apply the rounding formula with wager guard: w = score <= 0 ? 0 : min((score + 128) / 256, MAX_HIDERS)
            int32_t w = score.wager <= 0 ? 0 : (score.wager + 128) / 256;
            if (w > MAX_HIDERS) w = MAX_HIDERS;
            if (w >= 1) {
                applyWager(state, s, static_cast<uint8_t>(w), output);
            }
        }
    }

    // Close phase
    for (int s = 0; s < MAX_SERVERS; ++s) {
        const SeekerServerScores& score = scores.getOrDefault(s, SeekerServerScores{0, 0, 0});
        if ((state.seeker_connection_mask & (1u << s)) && score.close > 0) {
            state.seeker_connection_mask &= ~(1u << s);
            // Increment connection_tokens with saturation at uint8_t max (255)
            if (state.seeker.connection_tokens < 255) {
                ++state.seeker.connection_tokens;
            }
        }
    }

    // Open phase
    for (int open_iter = 0; open_iter < PLACEHOLDER_SEEKER_OPENS_PER_TICK && state.seeker.connection_tokens >= 1; ++open_iter) {
        int best_server = -1;
        int32_t best_score = 0;
        
        // Find the active, unconnected server with highest open score > 0
        for (int s = 0; s < MAX_SERVERS; ++s) {
            if (!state.servers[s].active) continue;
            if (state.seeker_connection_mask & (1u << s)) continue; // already connected
            
            const SeekerServerScores& score = scores.getOrDefault(s, SeekerServerScores{0, 0, 0});
            
            if (score.open <= 0) continue;
            
            if (score.open > best_score || best_server == -1) {
                best_score = score.open;
                best_server = s;
            }
        }
        
        if (best_server != -1) {
            state.seeker_connection_mask |= (1u << best_server);
            --state.seeker.connection_tokens;
        }
    }
}

} // namespace hider_seeker