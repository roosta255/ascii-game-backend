#include "ring_buffers.hpp"
#include <cstdint>

namespace hider_seeker {

void pushMetricFrame(ServerState& server, const MetricFrame& frame) {
    // Write frame into history[history_head % HISTORY_TICKS] and increment history_head
    uint8_t index = server.history_head % HISTORY_TICKS;
    server.history[index] = frame;
    server.history_head++;
}

RuleOutcome addPendingSeekerTarget(SeekerState& seeker, uint8_t serverId) {
    // Push into the target_ids/target_count/target_head ring buffer
    
    // Check if buffer is full
    if (seeker.target_count >= MAX_SEEKER_TARGETS) {
        // PROVISIONAL — NOT A RULESET-COMMITTED RING-002 DECISION.
        // RING-002 (seeker target overflow policy) is still an open question. Candidate
        // policies: (1) evict oldest, (2) retain highest-priority, (3) another named
        // option. Until the ruleset commits to one, this function rejects the new
        // target and leaves the buffer unchanged. Do not treat or document this
        // reject-on-full behavior as the final RING-002 rule; RING-002 remains
        // unresolved in job tracking and this branch must be revisited once decided.
        return RuleOutcome::REJECTED_NO_SLOT_AVAILABLE;
    }
    
    // Add new target
    uint8_t index = (seeker.target_head + seeker.target_count) % MAX_SEEKER_TARGETS;
    seeker.target_ids[index] = serverId;
    seeker.target_count++;
    
    return RuleOutcome::ACCEPTED;
}

} // namespace hider_seeker