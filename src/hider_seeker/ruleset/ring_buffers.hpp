#pragma once

#include "../domain/state.hpp"
#include "../../data/Array.hpp"
#include "rule_types.hpp"

namespace hider_seeker {

void pushMetricFrame(ServerState&, const MetricFrame& frame);
// RING-002 overflow policy: EVICT-OLDEST. Always ACCEPTED. When the
// target_ids/target_count/target_head ring buffer is already full
// (MAX_SEEKER_TARGETS), the oldest entry (at target_head) is evicted, the head
// advances by one, and serverId is stored as the newest entry; target_count
// stays at MAX_SEEKER_TARGETS. outEvictedServerId records the consequence: the
// evicted server id, or NO_SERVER_ID if nothing was evicted.
RuleOutcome addPendingSeekerTarget(SeekerState&, uint8_t serverId, uint8_t& outEvictedServerId);
// Same policy, for callers that do not need the eviction record.
RuleOutcome addPendingSeekerTarget(SeekerState&, uint8_t serverId);

} // namespace hider_seeker