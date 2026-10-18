#include "ScriptedSeekerPolicy.hpp"

namespace hider_seeker {

void ScriptedSeekerPolicy::score(const SeekerObservation&, Array<SeekerServerScores, MAX_SERVERS>& output) const {
    // Copy stored scores to output
    for (int s = 0; s < MAX_SERVERS; ++s) {
        output.access(s, [&](SeekerServerScores& out) {
            const auto& in = scores.getOrDefault(s, SeekerServerScores{});
            out.open = in.open;
            out.close = in.close;
            out.wager = in.wager;
        });
    }
}

std::string ScriptedSeekerPolicy::policyId() const {
    return "scripted";
}

} // namespace hider_seeker