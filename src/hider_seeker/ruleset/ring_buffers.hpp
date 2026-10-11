#pragma once

#include "../domain/state.hpp"
#include "../../data/Array.hpp"

namespace hider_seeker {

void pushMetricFrame(ServerState&, const MetricFrame& frame);
RuleOutcome addPendingSeekerTarget(SeekerState&, uint8_t serverId);

} // namespace hider_seeker