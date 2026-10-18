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
    // Process each server
    for (int s = 0; s < MAX_SERVERS; ++s) {
        // Only set scores for active servers; all other scores remain 0
        const auto& serverFeatures = obs.server.getOrDefault(s, Array<int16_t, SEEKER_SERVER_FEATURES>{});
        
        if (serverFeatures.getOrDefault(0, int16_t{0}) == 0) {
            // Server is inactive, skip scoring
            continue;
        }
        
        // Get the global features
        const auto& global = obs.global;
        
        // For connected servers
        if (serverFeatures.getOrDefault(1, int16_t{0}) == 256) {
            // Connected server
            
            // Compute activity: 3*|f[errors Δ2]| + 2*|f[network Δ2]| + 2*|f[heat Δ2]| + 1*|f[cpu Δ2]|
            const int16_t errorsDelta2 = serverFeatures.getOrDefault(21, int16_t{0}); // k=2 block, index 5 (errors)
            const int16_t networkDelta2 = serverFeatures.getOrDefault(19, int16_t{0}); // k=2 block, index 3 (network)
            const int16_t heatDelta2 = serverFeatures.getOrDefault(20, int16_t{0});   // k=2 block, index 4 (heat)
            const int16_t cpuDelta2 = serverFeatures.getOrDefault(16, int16_t{0});    // k=2 block, index 0 (cpu)
            
            int activity = 3 * std::abs(errorsDelta2) + 
                           2 * std::abs(networkDelta2) + 
                           2 * std::abs(heatDelta2) + 
                           1 * std::abs(cpuDelta2);
            
            // Set wager score
            int32_t wager = 0;
            if (activity >= PLACEHOLDER_BASELINE_WAGER_THRESHOLD && 
                serverFeatures.getOrDefault(37, int16_t{0}) >= PLACEHOLDER_BASELINE_REWAGER_GAP) {
                wager = 256; // Q8 value (256 = 1.0)
            }
            
            // Set close score
            int32_t close = 0;
            if (global.getOrDefault(0, int16_t{0}) == 0) { // g[0] == 0
                close = 256; // Q8 value (256 = 1.0)
            }
            
            // Set scores for this server
            scores.access(s, [&](SeekerServerScores& out) {
                out.wager = wager;
                out.close = close;
                out.open = 0; // Not connected case handled below
            });
        } else {
            // Not connected server
            
            // Set open score
            int32_t open = 0;
            if (global.getOrDefault(1, int16_t{0}) < PLACEHOLDER_BASELINE_MAX_OPEN * 16) { // g[1] < 2*16
                open = 256 + serverFeatures.getOrDefault(37, int16_t{0}); // Q8 value (256 = 1.0)
            }
            
            // Set scores for this server
            scores.access(s, [&](SeekerServerScores& out) {
                out.open = open;
                out.close = 0; // Not connected case
                out.wager = 0; // Not connected case
            });
        }
    }
}

std::string BaselineSeekerPolicy::policyId() const {
    return "baseline";
}

} // namespace hider_seeker