#pragma once

#include "../domain/state.hpp"
#include "../../data/Array.hpp"

namespace hider_seeker {

struct ProbeCandidate {
    uint8_t serverId;
    int32_t score; // Integer score, never float
};

class iTargetRankingPolicy {
public:
    virtual void rankTargets(const MatchState&, Array<ProbeCandidate, MAX_SERVERS>& outCandidates) const = 0;
    virtual ~iTargetRankingPolicy() = default;
};

} // namespace hider_seeker