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
        // Apply overflow rule - this is a placeholder for the undecided policy (RING-002)
        // The actual implementation should apply one of:
        // 1. Evict oldest
        // 2. Retain highest-priority
        // 3. Another named option
        
        // For now, we'll return REJECTED to indicate overflow
        // This is a placeholder - the real implementation will be determined by open_questions
        
        // NOTE: This behavior is provisional and not yet a committed ruleset decision for RING-002.
        // The final policy has not been decided and this code should be updated when that decision is made.
        
        return RuleOutcome::REJECTED_NO_SLOT_AVAILABLE;
    }
    
    // Add new target
    uint8_t index = (seeker.target_head + seeker.target_count) % MAX_SEEKER_TARGETS;
    seeker.target_ids[index] = serverId;
    seeker.target_count++;
    
    return RuleOutcome::ACCEPTED;
}

} // namespace hider_seeker