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
    REQUIRE(match.event_count == 2);

    SECTION("missing, self, or inactive relocation target rejects with no state change") {
        uint8_t badTargets[] = {NO_SERVER_ID, 0, 5};
        for (uint8_t target : badTargets) {
            REQUIRE(applyRemoveServer(match, 0, 0, target) == RuleOutcome::REJECTED_RELOCATION_FAILED);
            REQUIRE(match.hiders[0].resources[0] == 1);
            REQUIRE(match.servers[0].active == 1);
            REQUIRE(match.servers[0].owner_hider_id == 0);
            REQUIRE(match.server_count == 2);
            REQUIRE(match.hiders[0].server_id == 0);
            REQUIRE(match.event_count == 2);
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
        REQUIRE(match.event_count == 3);
        REQUIRE(match.events[2].type == static_cast<uint8_t>(HiderSeekerEventType::SERVER_REMOVED));
    }
}

TEST_CASE("applyAddServer emits a public SERVER_ADDED lifecycle event", "[loa]") {
    MatchState match = {};
    match.tick = 42;
    match.hiders[0].resources[0] = 1;

    REQUIRE(applyAddServer(match, 0) == RuleOutcome::ACCEPTED);
    REQUIRE(match.event_count == 1);
    REQUIRE(match.events[0].type == static_cast<uint8_t>(HiderSeekerEventType::SERVER_ADDED));
    REQUIRE(match.events[0].tick == 42);
}

TEST_CASE("applyRemoveServer cancels seeker targets, slot seeker state and goals referencing the slot", "[loa]") {
    MatchState match = {};
    match.hiders[0].resources[0] = 1;
    match.servers[0].active = 1;
    match.servers[0].owner_hider_id = 0;
    match.servers[1].active = 1;
    match.servers[1].owner_hider_id = 0;
    match.server_count = 2;

    match.seeker.target_ids[0] = 0;
    match.seeker.target_ids[1] = 1;
    match.seeker.target_ids[2] = 0;
    match.seeker.target_ids[3] = 2;
    match.seeker.target_head = 0;
    match.seeker.target_count = 4;
    match.seeker.belief[0] = 7;
    match.seeker.last_probe_tick[0] = 9;

    match.goals[0].active = 1;
    match.goals[0].requirement_count = 1;
    match.goals[0].requirements[0].type = REQ_CONFIGURED_SERVER;
    match.goals[0].requirements[0].subject_id = 0;

    match.goals[1].active = 1; // subject_id is a resource type here, not a server
    match.goals[1].requirement_count = 1;
    match.goals[1].requirements[0].type = REQ_RESOURCE_MIN;
    match.goals[1].requirements[0].subject_id = 0;

    match.goals[2].active = 1; // references a different server
    match.goals[2].requirement_count = 1;
    match.goals[2].requirements[0].type = REQ_SERVER_METRIC_THRESHOLD;
    match.goals[2].requirements[0].subject_id = 1;

    REQUIRE(applyRemoveServer(match, 0, 0) == RuleOutcome::ACCEPTED);

    REQUIRE(match.seeker.target_count == 2);
    REQUIRE(match.seeker.target_ids[match.seeker.target_head % MAX_SEEKER_TARGETS] == 1);
    REQUIRE(match.seeker.target_ids[(match.seeker.target_head + 1) % MAX_SEEKER_TARGETS] == 2);
    REQUIRE(match.seeker.belief[0] == 0);
    REQUIRE(match.seeker.last_probe_tick[0] == 0);

    REQUIRE(match.goals[0].active == 0);
    REQUIRE(match.goals[1].active == 1);
    REQUIRE(match.goals[2].active == 1);
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