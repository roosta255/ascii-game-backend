#include "baseline_policy.hpp"
#include <algorithm>

// PLACEHOLDER pending BALANCE-001
constexpr int PLACEHOLDER_BASELINE_WAGER_THRESHOLD = 20;
// PLACEHOLDER pending BALANCE-001
constexpr int PLACEHOLDER_BASELINE_REWAGER_GAP = 16;
// PLACEHOLDER pending BALANCE-001
constexpr int PLACEHOLDER_BASELINE_MAX_OPEN = 2;

void BaselineSeekerPolicy::score(const SeekerObservation& observation, Array<SeekerServerScores, MAX_SERVERS>& scores) const {
    // Iterate over all servers - just set everything to zero for now to avoid compilation issues
    for (int s = 0; s < MAX_SERVERS; ++s) {
        scores.access(s, [&](SeekerServerScores& out) {
            out.open = 0;
            out.close = 0;
            out.wager = 0;
        });
    }
}

std::string BaselineSeekerPolicy::policyId() const {
    return "baseline";
}