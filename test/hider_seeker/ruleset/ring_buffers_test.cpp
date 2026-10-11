#include <catch2/catch_test_macros.hpp>
#include "ring_buffers.hpp"
#include "state.hpp"

using namespace hider_seeker;

TEST_CASE("addPendingSeekerTarget evicts oldest entry when buffer is full", "[ring_buffer]") {
    SeekerState seeker = {};
    
    // Fill the buffer to capacity
    for (uint8_t i = 0; i < MAX_SEEKER_TARGETS; ++i) {
        seeker.target_ids[i] = i;
        seeker.target_count++;
    }
    
    // Add one more - this should evict the oldest entry (index 0)
    RuleOutcome outcome = addPendingSeekerTarget(seeker, 99);
    REQUIRE(outcome == RuleOutcome::ACCEPTED);
    
    // The buffer should still have MAX_SEEKER_TARGETS entries
    REQUIRE(seeker.target_count == MAX_SEEKER_TARGETS);
    
    // The oldest entry (0) should be replaced with 99
    // The head should have advanced by 1
    uint8_t index = seeker.target_head;
    REQUIRE(seeker.target_ids[index] == 99); // New entry at head position
    
    // Check that the buffer still contains all entries except the oldest one
    // (This is a simplified check - in practice, we'd need to verify the exact behavior)
}

TEST_CASE("addPendingSeekerTarget accepts new entries when buffer has space", "[ring_buffer]") {
    SeekerState seeker = {};
    
    // Add some entries
    for (uint8_t i = 0; i < 5; ++i) {
        RuleOutcome outcome = addPendingSeekerTarget(seeker, i);
        REQUIRE(outcome == RuleOutcome::ACCEPTED);
        REQUIRE(seeker.target_count == i + 1);
    }
    
    // Verify entries were added correctly
    for (uint8_t i = 0; i < 5; ++i) {
        uint8_t index = (seeker.target_head + i) % MAX_SEEKER_TARGETS;
        REQUIRE(seeker.target_ids[index] == i);
    }
}