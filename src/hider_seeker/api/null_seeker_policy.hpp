#pragma once

#include "../ruleset/target_ranking_interface.hpp"

namespace hider_seeker {

// PLACEHOLDER pending SEEKER-005
class NullSeekerPolicy : public iTargetRankingPolicy {
public:
    void rankTargets(const MatchState&, Array<ProbeCandidate, MAX_SERVERS>& outCandidates) const override {
        // Writes nothing to any candidate slot, leaving every slot at the NO_SERVER_ID value
        // runTick pre-fills it with.
    }
};

} // namespace hider_seeker