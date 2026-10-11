#include <catch2/catch_test_macros.hpp>
#include "loa_rules.hpp"
#include "lifecycle_events.hpp"
#include "state.hpp"

using namespace hider_seeker;

TEST_CASE("applyAddServer spends exactly 1 LOA and activates a free slot", "[loa]") {
    MatchState match = {};
    
    // Initialize a hider with 2 LOA
    match.hiders[0].resources[0] = 2; // Assuming resource type 0 is LOA
    
    // Add server should succeed
    RuleOutcome outcome = applyAddServer(match, 0);
    REQUIRE(outcome == RuleOutcome::ACCEPTED);
    
    // Check that 1 LOA was spent
    REQUIRE(match.hiders[0].resources[0] == 1);
    
    // Check that a slot was activated
    REQUIRE(match.servers[0].active == 1);
    REQUIRE(match.servers[0].owner_hider_id == 0);
    REQUIRE(match.server_count == 1);
}

TEST_CASE("applyAddServer is rejected with zero LOA spent when no slot is free", "[loa]") {
    MatchState match = {};
    
    // Initialize a hider with 1 LOA
    match.hiders[0].resources[0] = 1;
    
    // Fill all servers
    for (int i = 0; i < MAX_SERVERS; ++i) {
        match.servers[i].active = 1;
        match.server_count++;
    }
    
    // Add server should fail
    RuleOutcome outcome = applyAddServer(match, 0);
    REQUIRE(outcome == RuleOutcome::REJECTED_NO_SLOT_AVAILABLE);
    
    // Check that no LOA was spent
    REQUIRE(match.hiders[0].resources[0] == 1);
}

TEST_CASE("applyAddServer is rejected with zero LOA spent when LOA < 1", "[loa]") {
    MatchState match = {};
    
    // Initialize a hider with 0 LOA
    match.hiders[0].resources[0] = 0;
    
    // Add server should fail
    RuleOutcome outcome = applyAddServer(match, 0);
    REQUIRE(outcome == RuleOutcome::REJECTED_INSUFFICIENT_RESOURCE);
    
    // Check that no LOA was spent
    REQUIRE(match.hiders[0].resources[0] == 0);
}

TEST_CASE("applyRemoveServer atomically relocates an occupant or is fully rejected", "[loa]") {
    MatchState match = {};
    
    // Initialize a hider with 2 LOA
    match.hiders[0].resources[0] = 2;
    
    // Add server first
    applyAddServer(match, 0);
    REQUIRE(match.hiders[0].resources[0] == 1);

    // Remove server should succeed (no active hider occupies the slot, so no relocation needed)
    RuleOutcome outcome = applyRemoveServer(match, 0, 0);
    REQUIRE(outcome == RuleOutcome::ACCEPTED);

    // Check that the removal itself spent exactly 1 LOA (REMOVE_SERVER costs 1 LOA per design §4.2)
    REQUIRE(match.hiders[0].resources[0] == 0);
    
    // Check that slot was deactivated
    REQUIRE(match.servers[0].active == 0);
    REQUIRE(match.server_count == 0);
}

TEST_CASE("applyRemoveServer on an occupied slot relocates atomically or is fully rejected", "[loa]") {
    MatchState match = {};
    match.hiders[0].active = 1;
    match.hiders[0].resources[0] = 3;

    REQUIRE(applyAddServer(match, 0) == RuleOutcome::ACCEPTED); // slot 0
    REQUIRE(applyAddServer(match, 0) == RuleOutcome::ACCEPTED); // slot 1
    match.hiders[0].server_id = 0; // hider occupies slot 0
    REQUIRE(match.hiders[0].resources[0] == 1);
    REQUIRE(match.server_count == 2);
    REQUIRE(match.event_count == 0); // lifecycle events are not wired into MatchState::events

    SECTION("missing, self, or inactive relocation target rejects with no state change") {
        uint8_t badTargets[] = {NO_SERVER_ID, 0, 5};
        for (uint8_t target : badTargets) {
            REQUIRE(applyRemoveServer(match, 0, 0, target) == RuleOutcome::REJECTED_RELOCATION_FAILED);
            REQUIRE(match.hiders[0].resources[0] == 1);
            REQUIRE(match.servers[0].active == 1);
            REQUIRE(match.servers[0].owner_hider_id == 0);
            REQUIRE(match.server_count == 2);
            REQUIRE(match.hiders[0].server_id == 0);
            REQUIRE(match.event_count == 0);
        }
    }

    SECTION("valid relocation target relocates the occupant in the same action") {
        REQUIRE(applyRemoveServer(match, 0, 0, 1) == RuleOutcome::ACCEPTED);
        REQUIRE(match.hiders[0].resources[0] == 0);
        REQUIRE(match.servers[0].active == 0);
        REQUIRE(match.servers[0].owner_hider_id == 0xFF);
        REQUIRE(match.servers[1].active == 1);
        REQUIRE(match.server_count == 1);
        REQUIRE(match.hiders[0].server_id == 1);
        REQUIRE(match.event_count == 0);
    }
}

TEST_CASE("applyAddServer does not write lifecycle events into MatchState::events", "[loa]") {
    MatchState match = {};
    match.tick = 42;
    match.hiders[0].resources[0] = 1;

    REQUIRE(applyAddServer(match, 0) == RuleOutcome::ACCEPTED);
    REQUIRE(match.event_count == 0);
    REQUIRE(match.event_head == 0);
}

TEST_CASE("applyRemoveServer invalidates every reference to the removed slot (LOA-004)", "[loa]") {
    MatchState match = {};
    match.hiders[0].active = 1;
    match.hiders[0].resources[LOA_RESOURCE_INDEX] = 3;
    REQUIRE(applyAddServer(match, 0) == RuleOutcome::ACCEPTED); // slot 0
    REQUIRE(applyAddServer(match, 0) == RuleOutcome::ACCEPTED); // slot 1
    match.hiders[0].server_id = 1;

    // Goal 0: configured-server requirement on slot 0 -> must be cancelled.
    match.goals[0].active = 1;
    match.goals[0].status = 3;
    match.goals[0].requirement_count = 2;
    match.goals[0].requirements[0] = {REQ_RESOURCE_MIN, 2, 5, 0};
    match.goals[0].requirements[1] = {REQ_CONFIGURED_SERVER, 0, 0, 0};
    match.hiders[0].goal_progress[0] = 7;
    // Goal 1: metric-threshold requirement on slot 1 -> untouched.
    match.goals[1].active = 1;
    match.goals[1].status = 3;
    match.goals[1].requirement_count = 1;
    match.goals[1].requirements[0] = {REQ_SERVER_METRIC_THRESHOLD, 1, 0, 50};
    match.hiders[0].goal_progress[1] = 4;
    // Goal 2: non-server requirement whose subject_id happens to be 0 -> untouched.
    match.goals[2].active = 1;
    match.goals[2].status = 3;
    match.goals[2].requirement_count = 1;
    match.goals[2].requirements[0] = {REQ_RESOURCE_SPEND, 0, 2, 0};

    // Pending seeker targets (oldest -> newest): 0, 1, 0, 2, with head at 14 (wraps).
    match.seeker.target_head = 14;
    match.seeker.target_ids[14] = 0;
    match.seeker.target_ids[15] = 1;
    match.seeker.target_ids[0] = 0;
    match.seeker.target_ids[1] = 2;
    match.seeker.target_count = 4;

    match.seeker.belief[0] = 9;
    match.seeker.last_probe_tick[0] = 5;
    match.seeker.belief[1] = 6;
    match.seeker.last_probe_tick[1] = 4;

    REQUIRE(applyRemoveServer(match, 0, 0) == RuleOutcome::ACCEPTED);

    // Goal referencing the removed slot is cancelled and its progress reset.
    REQUIRE(match.goals[0].active == 0);
    REQUIRE(match.goals[0].status == GOAL_STATUS_CANCELLED_SERVER_REMOVED);
    REQUIRE(match.hiders[0].goal_progress[0] == 0);
    // Goals not referencing the removed slot are unchanged.
    REQUIRE(match.goals[1].active == 1);
    REQUIRE(match.goals[1].status == 3);
    REQUIRE(match.hiders[0].goal_progress[1] == 4);
    REQUIRE(match.goals[2].active == 1);
    REQUIRE(match.goals[2].status == 3);

    // Pending targets for slot 0 are removed; survivors keep order from the head.
    REQUIRE(match.seeker.target_count == 2);
    REQUIRE(match.seeker.target_head == 14);
    REQUIRE(match.seeker.target_ids[14] == 1);
    REQUIRE(match.seeker.target_ids[15] == 2);

    // Seeker knowledge of slot 0 is reset; slot 1 is untouched.
    REQUIRE(match.seeker.belief[0] == 0);
    REQUIRE(match.seeker.last_probe_tick[0] == 0);
    REQUIRE(match.seeker.belief[1] == 6);
    REQUIRE(match.seeker.last_probe_tick[1] == 4);

    // A still-pending REMOVE_SERVER intent naming the removed slot is invalidated
    // at apply time, with no LOA spent.
    REQUIRE(match.hiders[0].resources[LOA_RESOURCE_INDEX] == 0);
    match.hiders[0].resources[LOA_RESOURCE_INDEX] = 1;
    REQUIRE(applyRemoveServer(match, 0, 0) == RuleOutcome::REJECTED_INVALID_TARGET);
    REQUIRE(match.hiders[0].resources[LOA_RESOURCE_INDEX] == 1);
}

TEST_CASE("resetServerSlot clears metrics/history/ownership/trace fields", "[loa]") {
    MatchState match = {};
    
    // Set up a server with data
    match.servers[0].active = 1;
    match.servers[0].owner_hider_id = 5;
    match.servers[0].trace_strength = 100;
    match.servers[0].trace_age = 20;
    match.servers[0].current.cpu = 50;
    match.servers[0].history[0].cpu = 40;
    match.servers[0].history_head = 1;
    
    // Reset the slot
    resetServerSlot(match.servers[0]);
    
    // Check that all fields are cleared
    REQUIRE(match.servers[0].active == 0);
    REQUIRE(match.servers[0].owner_hider_id == 0xFF);
    REQUIRE(match.servers[0].trace_strength == 0);
    REQUIRE(match.servers[0].trace_age == 0);
    REQUIRE(match.servers[0].current.cpu == 0);
    REQUIRE(match.servers[0].history[0].cpu == 0);
    REQUIRE(match.servers[0].history_head == 0);
}