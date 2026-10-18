#include "baseline_policy.hpp"
#include <algorithm>

// PLACEHOLDER pending BALANCE-001
constexpr int PLACEHOLDER_BASELINE_WAGER_THRESHOLD = 20;
// PLACEHOLDER pending BALANCE-001
constexpr int PLACEHOLDER_BASELINE_REWAGER_GAP = 16;
// PLACEHOLDER pending BALANCE-001
constexpr int PLACEHOLDER_BASELINE_MAX_OPEN = 2;

namespace hider_seeker {

void BaselineSeekerPolicy::score(const SeekerObservation& observation, Array<SeekerServerScores, MAX_SERVERS>& scores) const {
    // Process only active servers
    for (int s = 0; s < MAX_SERVERS; ++s) {
        scores.access(s, [&](SeekerServerScores& out) {
            out.open = 0;
            out.close = 0;
            out.wager = 0;
        });
    }
    
    // Process only active servers
    for (int s = 0; s < MAX_SERVERS; ++s) {
        // Get server features using accessConst to avoid modifying the observation
        const auto& serverFeatures = observation.server.getOrDefault(s, Array<int16_t, SEEKER_SERVER_FEATURES>{});
        
        // Only process if the server is active
        if (serverFeatures.getOrDefault(0, int16_t{0}) == 256) {  // active flag
            scores.access(s, [&](SeekerServerScores& out) {
                const bool isConnected = (serverFeatures.getOrDefault(1, int16_t{0}) == 256);  // connected flag
                
                if (isConnected) {
                    // Connected server logic
                    // activity = 3*abs(errors Δ2) + 2*abs(network Δ2) + 2*abs(heat Δ2) + 1*abs(cpu Δ2)
                    const int16_t errorsDelta2 = serverFeatures.getOrDefault(21, int16_t{0});  // index 21 is errors Δ2 (k=2)
                    const int16_t networkDelta2 = serverFeatures.getOrDefault(22, int16_t{0}); // index 22 is network Δ2
                    const int16_t heatDelta2 = serverFeatures.getOrDefault(23, int16_t{0});   // index 23 is heat Δ2
                    const int16_t cpuDelta2 = serverFeatures.getOrDefault(24, int16_t{0});    // index 24 is cpu Δ2
                    
                    const int activity = 3 * std::abs(errorsDelta2) + 
                                         2 * std::abs(networkDelta2) + 
                                         2 * std::abs(heatDelta2) + 
                                         1 * std::abs(cpuDelta2);
                    
                    // Set wager if activity >= threshold and ticks since last wager >= gap
                    const int16_t ticksSinceWager = serverFeatures.getOrDefault(37, int16_t{0});
                    if (activity >= PLACEHOLDER_BASELINE_WAGER_THRESHOLD && 
                        ticksSinceWager >= PLACEHOLDER_BASELINE_REWAGER_GAP) {
                        out.wager = 256;
                    }
                    
                    // Set close if pool is empty
                    const auto& globalFeatures = observation.global;
                    const int16_t pool = globalFeatures.getOrDefault(0, int16_t{0});
                    if (pool == 0) {
                        out.close = 256;
                    }
                } else {
                    // Not connected server logic
                    const int16_t ticksSinceWager = serverFeatures.getOrDefault(37, int16_t{0});
                    const auto& globalFeatures = observation.global;
                    const int16_t openConnections = globalFeatures.getOrDefault(1, int16_t{0});
                    
                    // Open if we have fewer than max open connections
                    if (openConnections < PLACEHOLDER_BASELINE_MAX_OPEN * 16) {
                        out.open = 256 + ticksSinceWager;
                    }
                }
            });
        }
    }
}

std::string BaselineSeekerPolicy::policyId() const {
    return "baseline";
}

} // namespace hider_seeker