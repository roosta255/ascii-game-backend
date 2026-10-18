#include "baseline_policy.hpp"
#include <algorithm>

namespace hider_seeker {

// PLACEHOLDER pending BALANCE-001
constexpr int PLACEHOLDER_BASELINE_WAGER_THRESHOLD = 20;
// PLACEHOLDER pending BALANCE-001
constexpr int PLACEHOLDER_BASELINE_REWAGER_GAP = 16;
// PLACEHOLDER pending BALANCE-001
constexpr int PLACEHOLDER_BASELINE_MAX_OPEN = 2;

void BaselineSeekerPolicy::score(const SeekerObservation& obs, Array<SeekerServerScores, MAX_SERVERS>& scores) const {
    // Iterate over all servers
    for (int s = 0; s < MAX_SERVERS; ++s) {
        // Only process active servers
        bool isActive = false;
        obs.server.accessConst(s, [&](const Array<int16_t, SEEKER_SERVER_FEATURES>& row) {
            isActive = (row.getOrDefault(0, int16_t{0}) == 256);
        });
        
        if (!isActive) {
            // Server is inactive, leave scores at 0
            continue;
        }
        
        // Get server features
        Array<int16_t, SEEKER_SERVER_FEATURES> server_features;
        obs.server.accessConst(s, [&](const Array<int16_t, SEEKER_SERVER_FEATURES>& row) {
            server_features = row;
        });
        
        const bool connected = (server_features.getOrDefault(1, int16_t{0}) == 256);
        
        SeekerServerScores result{0, 0, 0}; // Initialize all scores to 0
        
        if (connected) {
            // Connected server: compute activity and set wager/close scores
            const int16_t errorsDelta2 = server_features.getOrDefault(21, int16_t{0});  // k=2 block, start 16, pos 5, sum 21
            const int16_t networkDelta2 = server_features.getOrDefault(19, int16_t{0}); // k=2 block, start 16, pos 3, sum 19
            const int16_t heatDelta2 = server_features.getOrDefault(20, int16_t{0});   // k=2 block, start 16, pos 4, sum 20
            const int16_t cpuDelta2 = server_features.getOrDefault(16, int16_t{0});    // k=2 block, start 16, pos 0, sum 16
            
            // Compute activity: 3*|errors Δ2| + 2*|network Δ2| + 2*|heat Δ2| + 1*|cpu Δ2|
            const int activity = 3 * std::abs(errorsDelta2) + 
                                2 * std::abs(networkDelta2) + 
                                2 * std::abs(heatDelta2) + 
                                1 * std::abs(cpuDelta2);
            
            // Set wager score
            const int16_t ticksSinceLastWager = server_features.getOrDefault(37, int16_t{0});
            if (activity >= PLACEHOLDER_BASELINE_WAGER_THRESHOLD && 
                ticksSinceLastWager >= PLACEHOLDER_BASELINE_REWAGER_GAP) {
                result.wager = 256; // wager = 256 (1.0)
            }
            
            // Set close score
            int16_t seekerPool = 0;
            obs.global.accessConst(0, [&](const int16_t& value) { seekerPool = value; });
            if (seekerPool == 0) {
                result.close = 256; // close = 256 (1.0)
            }
        } else {
            // Not connected server: set open score if appropriate
            const int16_t ticksSinceLastWager = server_features.getOrDefault(37, int16_t{0});
            int16_t openConnections = 0;
            obs.global.accessConst(1, [&](const int16_t& value) { openConnections = value; });
            
            if (openConnections < PLACEHOLDER_BASELINE_MAX_OPEN * 16) {
                // open = 256 + ticksSinceLastWager
                result.open = 256 + ticksSinceLastWager;
            }
        }
        
        scores.set(s, result);
    }
}

std::string BaselineSeekerPolicy::policyId() const {
    return "baseline";
}

} // namespace hider_seeker