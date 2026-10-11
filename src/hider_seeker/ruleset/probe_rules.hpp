#pragma once

#include "rule_types.hpp"
#include "../domain/state.hpp"
#include "target_ranking_interface.hpp"

namespace hider_seeker {

struct ProbeResult {
    bool hit;
    uint8_t targetServerId;
};

RuleOutcome applyProbe(MatchState&, uint8_t serverId, ProbeResult& outResult);
void resolveProbeConsequences(MatchState&, const ProbeResult&);

} // namespace hider_seeker