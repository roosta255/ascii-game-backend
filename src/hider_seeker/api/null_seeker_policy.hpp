#pragma once

#include "../ruleset/target_ranking_interface.hpp"

namespace hider_seeker {

// PLACEHOLDER pending SEEKER-005: a seeker policy that ranks nothing. It
// writes no candidate slot, so every slot keeps the NO_SERVER_ID value runTick
// pre-fills it with and no probe is ever issued.
class NullSeekerPolicy : public iTargetRankingPolicy {
public:
    void rankTargets(const MatchState&, Array<ProbeCandidate, MAX_SERVERS>&) const override {}
};

} // namespace hider_seeker