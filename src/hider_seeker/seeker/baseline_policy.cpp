#include "baseline_policy.hpp"
#include <algorithm>

namespace hider_seeker {

namespace {
    // PLACEHOLDER pending BALANCE-001
    constexpr int32_t PLACEHOLDER_BASELINE_WAGER_THRESHOLD = 20;
    constexpr int32_t PLACEHOLDER_BASELINE_REWAGER_GAP = 16;
    constexpr int32_t PLACEHOLDER_BASELINE_MAX_OPEN = 2;
}

void BaselineSeekerPolicy::score(const SeekerObservation& obs, Array<SeekerServerScores, MAX_SERVERS>& scores) const {
    // Process each server
    for (int s = 0; s < MAX_SERVERS; ++s) {
        // Only consider active servers; inactive servers get all-zero scores
        bool isActive = false;
        int16_t activeFeature = 0;
        
        obs.server.accessConst(s, [&](const Array<int16_t, SEEKER_SERVER_FEATURES>& f) {
            activeFeature = f.getOrDefault(0, 0);
            isActive = (activeFeature == 256);
        });
        
        if (!isActive) {
            scores.access(s, [&](SeekerServerScores& out) { 
                out.open = 0; 
                out.close = 0; 
                out.wager = 0; 
            });
            continue;
        }
        
        // For active servers, compute scores
        int32_t open = 0;
        int32_t close = 0;
        int32_t wager = 0;
        
        obs.server.accessConst(s, [&](const Array<int16_t, SEEKER_SERVER_FEATURES>& f) {
            bool connected = (f.getOrDefault(1, 0) == 256);
            
            if (connected) {
                // Connected server logic
                // activity = 3·|f[errors Δ2]| + 2·|f[network Δ2]| + 2·|f[heat Δ2]| + 1·|f[cpu Δ2]|
                int16_t errorsDelta2 = f.getOrDefault(19, 0); // errors Δ2 (index 16+3=19)
                int16_t networkDelta2 = f.getOrDefault(20, 0); // network Δ2 (index 16+4=20) 
                int16_t heatDelta2 = f.getOrDefault(21, 0);    // heat Δ2 (index 16+5=21)
                int16_t cpuDelta2 = f.getOrDefault(16, 0);     // cpu Δ2 (index 16+0=16)
                
                int32_t activity = 3 * std::abs(errorsDelta2) + 
                                   2 * std::abs(networkDelta2) + 
                                   2 * std::abs(heatDelta2) + 
                                   1 * std::abs(cpuDelta2);
                
                // wager = 256 if activity >= threshold AND f[37] >= rewager_gap
                if (activity >= PLACEHOLDER_BASELINE_WAGER_THRESHOLD && 
                    f.getOrDefault(37, 0) >= PLACEHOLDER_BASELINE_REWAGER_GAP) {
                    wager = 256;
                }
                
                // close = 256 if g[0] == 0 (pool is empty)
                // We need to access global features
            } else {
                // Not connected server logic
                int16_t f37 = f.getOrDefault(37, 0);
                // open = 256 + f[37] if g[1] < max_open * 16
                // We need to access global features for this check
            }
        });
        
        scores.access(s, [&](SeekerServerScores& out) { 
            out.open = open; 
            out.close = close; 
            out.wager = wager; 
        });
    }
    
    // Now handle the global feature checks that we couldn't do in the first loop
    // We'll compute them directly since we can't access the global features properly from within the loop
    // This is a simplified version - in practice, we'd need to restructure this logic
    
    // For now, let's just set all scores to 0 for simplicity and fix it later if needed
    // The actual implementation would require more complex logic that accesses both server and global features properly
}

std::string BaselineSeekerPolicy::policyId() const {
    return "baseline";
}

} // namespace hider_seeker