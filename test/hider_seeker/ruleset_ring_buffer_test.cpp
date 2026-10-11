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
    
    REQUIRE(server.history_head == HISTORY_TICKS + 1);

    // Slot 0 held the oldest frame (cpu == 0); it must now hold the newest one
    // (cpu == HISTORY_TICKS), while every other slot still holds frame i.
    REQUIRE(server.history[0].cpu == HISTORY_TICKS);
    for (int i = 1; i < HISTORY_TICKS; ++i) {
        REQUIRE(server.history[i].cpu == i);
    }
}

TEST_CASE("addPendingSeekerTarget appends in ring order below capacity", "[ring]") {
    SeekerState seeker = {};
    seeker.target_head = MAX_SEEKER_TARGETS - 1; // force wrap-around on the 2nd push

    REQUIRE(addPendingSeekerTarget(seeker, 3) == RuleOutcome::ACCEPTED);
    REQUIRE(addPendingSeekerTarget(seeker, 9) == RuleOutcome::ACCEPTED);

    REQUIRE(seeker.target_count == 2);
    REQUIRE(seeker.target_ids[MAX_SEEKER_TARGETS - 1] == 3);
    REQUIRE(seeker.target_ids[0] == 9);
}

TEST_CASE("addPendingSeekerTarget never exceeds capacity when full", "[ring]") {
    // RING-002 (which overflow policy applies) is an open ruleset question, so
    // this only checks invariants that must hold under ANY overflow policy.
    SeekerState seeker = {};
    for (int i = 0; i < MAX_SEEKER_TARGETS; ++i) {
        seeker.target_ids[i] = static_cast<uint8_t>(i);
        seeker.target_count++;
    }

    addPendingSeekerTarget(seeker, 10);

    REQUIRE(seeker.target_count == MAX_SEEKER_TARGETS);
    REQUIRE(seeker.target_head < MAX_SEEKER_TARGETS);
}