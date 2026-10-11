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
        // RING-002: Implement explicit overflow policy - evict oldest entry
        // This follows a deterministic, ruleset-defined behavior instead of just rejecting
        
        // Evict the oldest entry (at target_head)
        seeker.target_head = (seeker.target_head + 1) % MAX_SEEKER_TARGETS;
        // The count remains the same since we're replacing an element
    }
    
    // Add new target at the end of the buffer
    uint8_t index = (seeker.target_head + seeker.target_count) % MAX_SEEKER_TARGETS;
    seeker.target_ids[index] = serverId;
    
    // Only increment count if we didn't overwrite an existing entry
    if (seeker.target_count < MAX_SEEKER_TARGETS) {
        seeker.target_count++;
    }
    
    return RuleOutcome::ACCEPTED;
}

} // namespace hider_seeker