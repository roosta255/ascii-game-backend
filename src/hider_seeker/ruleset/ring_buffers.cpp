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
        // RING-002 (seeker target overflow policy: reject vs overwrite-oldest vs
        // other) is an OPEN ruleset question and is NOT decided here. This branch
        // is only a memory-safety guard so the buffer is never written past
        // capacity; it is not the ruleset default, and no test asserts it as one.
        // Replace this branch once RING-002 is resolved through the open-question
        // process.
        return RuleOutcome::REJECTED_NO_SLOT_AVAILABLE;
    }
    
    // Add new target
    uint8_t index = (seeker.target_head + seeker.target_count) % MAX_SEEKER_TARGETS;
    seeker.target_ids[index] = serverId;
    seeker.target_count++;
    
    return RuleOutcome::ACCEPTED;
}

} // namespace hider_seeker