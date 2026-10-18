#include "baseline_policy.hpp"
#include <algorithm>

namespace hider_seeker {

namespace {
    // PLACEHOLDER pending BALANCE-001
    constexpr int PLACEHOLDER_BASELINE_WAGER_THRESHOLD = 20;
    constexpr int PLACEHOLDER_BASELINE_REWAGER_GAP = 16;
    constexpr int PLACEHOLDER_BASELINE_MAX_OPEN = 2;
} // anonymous namespace

void BaselineSeekerPolicy::score(const SeekerObservation& obs, Array<SeekerServerScores, MAX_SERVERS>& out) const {
    // Initialize all scores to 0
    for (auto& score : out) {
        score = SeekerServerScores{};
    }
    
    for (int s = 0; s < MAX_SERVERS; ++s) {
        // Only process active servers
        if (obs.server.getOrDefault(s, {}).getOrDefault(0, int16_t{0}) == 256) {
            const auto& serverFeatures = obs.server.getOrDefault(s, {});
            
            if (serverFeatures.getOrDefault(1, int16_t{0}) == 256) {
                // Server is connected - compute wager score
                const int16_t errorsDelta2 = serverFeatures.getOrDefault(21, int16_t{0});  // index 16 + 5 (for errors)
                const int16_t networkDelta2 = serverFeatures.getOrDefault(22, int16_t{0}); // index 16 + 6 (for network)
                const int16_t heatDelta2 = serverFeatures.getOrDefault(20, int16_t{0});   // index 16 + 4 (for heat)
                const int16_t cpuDelta2 = serverFeatures.getOrDefault(16, int16_t{0});    // index 16 + 0 (for cpu)
                
                const int activity = 3 * std::abs(errorsDelta2) + 
                                     2 * std::abs(networkDelta2) + 
                                     2 * std::abs(heatDelta2) + 
                                     1 * std::abs(cpuDelta2);
                
                // Set wager score
                if (activity >= PLACEHOLDER_BASELINE_WAGER_THRESHOLD && 
                    serverFeatures.getOrDefault(37, int16_t{0}) >= PLACEHOLDER_BASELINE_REWAGER_GAP) {
                    out.access(s, [](SeekerServerScores& scores) { scores.wager = 256; });
                } else {
                    out.access(s, [](SeekerServerScores& scores) { scores.wager = 0; });
                }
                
                // Set close score
                if (obs.global.getOrDefault(0, int16_t{0}) == 0) {
                    out.access(s, [](SeekerServerScores& scores) { scores.close = 256; });
                } else {
                    out.access(s, [](SeekerServerScores& scores) { scores.close = 0; });
                }
            } else {
                // Server is not connected - compute open score
                const int16_t ticksSinceLastWager = serverFeatures.getOrDefault(37, int16_t{0});
                if (obs.global.getOrDefault(1, int16_t{0}) < PLACEHOLDER_BASELINE_MAX_OPEN * 16) {
                    out.access(s, [ticksSinceLastWager](SeekerServerScores& scores) { scores.open = 256 + ticksSinceLastWager; });
                } else {
                    out.access(s, [](SeekerServerScores& scores) { scores.open = 0; });
                }
            }
        }
    }
}

std::string BaselineSeekerPolicy::policyId() const {
    return "baseline";
}

} // namespace hider_seeker