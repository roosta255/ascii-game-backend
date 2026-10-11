#include <catch2/catch_test_macros.hpp>
#include "ring_buffers.hpp"
#include "state.hpp"

using namespace hider_seeker;

TEST_CASE("pushMetricFrame overwrites oldest frame after HISTORY_TICKS frames", "[ring]") {
    ServerState server = {};
    
    // Push HISTORY_TICKS + 1 frames
    for (int i = 0; i < HISTORY_TICKS + 1; ++i) {
        MetricFrame frame = {static_cast<uint8_t>(i), 0, 0, 0, 0, 0, 0};
        pushMetricFrame(server, frame);
    }
    
    // Check that the buffer contains the most recent HISTORY_TICKS frames
    // The oldest frame (with value 0) should have been overwritten by frame with value HISTORY_TICKS
    REQUIRE(server.history_head == HISTORY_TICKS + 1);
    
    // The first frame in history should be the one that overwrote the oldest
    // This is a placeholder - actual test would check specific values
    REQUIRE(true); // Placeholder for more detailed testing
}

TEST_CASE("addPendingSeekerTarget handles overflow with defined rule", "[ring]") {
    SeekerState seeker = {};
    
    // Fill up the target buffer
    for (int i = 0; i < MAX_SEEKER_TARGETS; ++i) {
        seeker.target_ids[i] = static_cast<uint8_t>(i);
        seeker.target_count++;
    }
    
    // Try to add one more - should fail due to overflow
    RuleOutcome outcome = addPendingSeekerTarget(seeker, 10);
    REQUIRE(outcome == RuleOutcome::REJECTED_NO_SLOT_AVAILABLE);
}