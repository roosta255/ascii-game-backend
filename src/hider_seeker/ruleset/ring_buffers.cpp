#include "ring_buffers.hpp"
#include <cstdint>

namespace hider_seeker {

void pushMetricFrame(ServerState& server, const MetricFrame& frame) {
    // Write frame into history[history_head % HISTORY_TICKS] and increment history_head
    uint8_t index = server.history_head % HISTORY_TICKS;
    server.history[index] = frame;
    server.history_head++;
}

RuleOutcome addPendingSeekerTarget(SeekerState& seeker, uint8_t serverId, uint8_t& outEvictedServerId) {
    // RING-002: evict-oldest overflow policy (see ring_buffers.hpp).
    // TODO: This is a placeholder for an undecided design choice (RING-002).
    // See implementation plan's open_questions list.
    outEvictedServerId = NO_SERVER_ID;
    seeker.target_head = static_cast<uint8_t>(seeker.target_head % MAX_SEEKER_TARGETS);

    if (seeker.target_count >= MAX_SEEKER_TARGETS) {
        // Full: the oldest entry sits at target_head. Record it, then write the
        // new entry into that same slot and advance the head past it, so the
        // new entry becomes the newest and the next-oldest becomes the head.
        seeker.target_count = MAX_SEEKER_TARGETS;
        outEvictedServerId = seeker.target_ids[seeker.target_head];
        seeker.target_ids[seeker.target_head] = serverId;
        seeker.target_head = static_cast<uint8_t>((seeker.target_head + 1) % MAX_SEEKER_TARGETS);
        return RuleOutcome::ACCEPTED;
    }

    const uint8_t index = static_cast<uint8_t>((seeker.target_head + seeker.target_count) % MAX_SEEKER_TARGETS);
    seeker.target_ids[index] = serverId;
    seeker.target_count++;
    return RuleOutcome::ACCEPTED;
}

RuleOutcome addPendingSeekerTarget(SeekerState& seeker, uint8_t serverId) {
    uint8_t evicted = NO_SERVER_ID;
    return addPendingSeekerTarget(seeker, serverId, evicted);
}

} // namespace hider_seeker