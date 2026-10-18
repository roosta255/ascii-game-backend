#include "baseline_policy.hpp"
#include <algorithm>

namespace hider_seeker {

std::string BaselineSeekerPolicy::policyId() const { 
    return "baseline"; 
}

// Scores only active servers; inactive servers keep scores at 0.
void BaselineSeekerPolicy::score(const SeekerObservation& obs, Array<SeekerServerScores, MAX_SERVERS>& out) const {
    for (size_t s = 0; s < MAX_SERVERS; ++s) {
        // Check if server is active using accessConst
        bool isActive = false;
        obs.server.accessConst(s, [&](const Array<int16_t, SEEKER_SERVER_FEATURES>& f_const) {
            isActive = (f_const.getOrDefault(0, int16_t{0}) != 0);
        });
        
        if (!isActive) continue; // server inactive
        
        // Get the feature array for this server
        Array<int16_t, SEEKER_SERVER_FEATURES> f;
        obs.server.accessConst(s, [&](const Array<int16_t, SEEKER_SERVER_FEATURES>& f_const) {
            f = f_const;
        });
        
        const auto& g = obs.global;
        
        // Connected flag is index 1.
        bool connected = (f.getOrDefault(1, int16_t{0}) != 0);
        
        // Compute activity if connected.
        int32_t activity = 0;
        if (connected) {
            // |f[errors Δ2]| etc. are absolute Q8 values.
            int16_t errorsDelta = f.getOrDefault(7, int16_t{0});
            int16_t heatDelta = f.getOrDefault(6, int16_t{0});  // Note: heat is index 6, errors index 7
            int16_t networkDelta = f.getOrDefault(5, int16_t{0});
            int16_t cpuDelta = f.getOrDefault(2, int16_t{0});
            
            activity += 3 * std::abs(errorsDelta); // errors (index 7)
            activity += 2 * std::abs(heatDelta);   // heat (index 6) 
            activity += 2 * std::abs(networkDelta); // network (index 5)
            activity += 1 * std::abs(cpuDelta);    // cpu (index 2)
        }
        
        // Hardcoded constants as specified in design
        constexpr int32_t BASELINE_WAGER_THRESHOLD = 20;
        constexpr int32_t BASELINE_REWAGER_GAP = 16;
        constexpr int32_t BASELINE_MAX_OPEN = 2;
        int16_t f37 = f.getOrDefault(37, int16_t{0});
        if (connected && activity >= BASELINE_WAGER_THRESHOLD && f37 >= BASELINE_REWAGER_GAP) {
            out.access(s, [&](SeekerServerScores& score) {
                score.wager = 256;
            });
        } else {
            out.access(s, [&](SeekerServerScores& score) {
                score.wager = 0;
            });
        }
        
        // Close score.
        int16_t g0 = g.getOrDefault(0, int16_t{0});
        out.access(s, [&](SeekerServerScores& score) {
            score.close = (g0 == 0) ? 256 : 0;
        });
        
        // Open score when not connected.
        if (!connected) {
            // BASELINE_MAX_OPEN * 16 = 2 * 16 = 32
            int16_t g1 = g.getOrDefault(1, int16_t{0});
            int16_t f37_val = f.getOrDefault(37, int16_t{0});
            out.access(s, [&](SeekerServerScores& score) {
                score.open = (g1 < (BASELINE_MAX_OPEN * 16)) ? (256 + f37_val) : 0;
            });
        } else {
            out.access(s, [&](SeekerServerScores& score) {
                score.open = 0;
            });
        }
    }
}

} // namespace hider_seeker