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

TEST_CASE("addPendingSeekerTarget below capacity reports no eviction", "[ring]") {
    SeekerState seeker = {};
    uint8_t evicted = 0;
    REQUIRE(addPendingSeekerTarget(seeker, 4, evicted) == RuleOutcome::ACCEPTED);
    REQUIRE(evicted == NO_SERVER_ID);
    REQUIRE(seeker.target_count == 1);
    REQUIRE(seeker.target_head == 0);
    REQUIRE(seeker.target_ids[0] == 4);
}

TEST_CASE("addPendingSeekerTarget evicts the oldest entry when full (RING-002)", "[ring]") {
    // Full buffer whose head is not at index 0, so wrap-around is exercised.
    // Logical order (oldest -> newest) is 100, 101, ..., 115.
    SeekerState seeker = {};
    const uint8_t startHead = 5;
    seeker.target_head = startHead;
    for (int i = 0; i < MAX_SEEKER_TARGETS; ++i) {
        seeker.target_ids[(startHead + i) % MAX_SEEKER_TARGETS] = static_cast<uint8_t>(100 + i);
    }
    seeker.target_count = MAX_SEEKER_TARGETS;

    uint8_t evicted = NO_SERVER_ID;
    REQUIRE(addPendingSeekerTarget(seeker, 7, evicted) == RuleOutcome::ACCEPTED);

    // The recorded consequence is the evicted oldest entry.
    REQUIRE(evicted == 100);
    REQUIRE(seeker.target_count == MAX_SEEKER_TARGETS);
    REQUIRE(seeker.target_head == startHead + 1);

    // New logical order: 101, ..., 115, 7 (newest last).
    for (int i = 0; i < MAX_SEEKER_TARGETS - 1; ++i) {
        REQUIRE(seeker.target_ids[(seeker.target_head + i) % MAX_SEEKER_TARGETS] == 101 + i);
    }
    REQUIRE(seeker.target_ids[(seeker.target_head + MAX_SEEKER_TARGETS - 1) % MAX_SEEKER_TARGETS] == 7);

    // A second overflow evicts the next-oldest entry.
    REQUIRE(addPendingSeekerTarget(seeker, 8, evicted) == RuleOutcome::ACCEPTED);
    REQUIRE(evicted == 101);
    REQUIRE(seeker.target_count == MAX_SEEKER_TARGETS);
    REQUIRE(seeker.target_ids[(seeker.target_head + MAX_SEEKER_TARGETS - 1) % MAX_SEEKER_TARGETS] == 8);
    REQUIRE(seeker.target_ids[(seeker.target_head + MAX_SEEKER_TARGETS - 2) % MAX_SEEKER_TARGETS] == 7);
}