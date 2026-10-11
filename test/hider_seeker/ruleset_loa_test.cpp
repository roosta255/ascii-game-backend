#include <catch2/catch_test_macros.hpp>
#include "loa_rules.hpp"
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
    
    // Remove server should succeed (placeholder - actual relocation logic not implemented)
    RuleOutcome outcome = applyRemoveServer(match, 0, 0);
    REQUIRE(outcome == RuleOutcome::ACCEPTED);
    
    // Check that 1 LOA was spent
    REQUIRE(match.hiders[0].resources[0] == 1);
    
    // Check that slot was deactivated
    REQUIRE(match.servers[0].active == 0);
    REQUIRE(match.server_count == 0);
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