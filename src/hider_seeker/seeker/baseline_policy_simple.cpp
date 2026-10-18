#include "baseline_policy.hpp"

void BaselineSeekerPolicy::score(const SeekerObservation& observation, Array<SeekerServerScores, MAX_SERVERS>& scores) const {
    // Simple implementation that just sets everything to zero to avoid compilation issues
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