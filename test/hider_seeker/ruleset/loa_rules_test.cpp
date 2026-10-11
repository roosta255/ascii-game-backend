#include <catch2/catch_test_macros.hpp>
#include "loa_rules.hpp"
#include "state.hpp"

using namespace hider_seeker;

TEST_CASE("applyRemoveServer invalidates references to removed server", "[loa]") {
    MatchState match = {};
    
    // Set up a hider with LOA
    match.hiders[0].resources[0] = 2; // LOA resource at index 0
    
    // Add a server first
    RuleOutcome outcome = applyAddServer(match, 0);
    REQUIRE(outcome == RuleOutcome::ACCEPTED);
    REQUIRE(match.server_count == 1);
    
    // Set up a goal with a requirement that references the server
    match.goals[0].active = 1;
    match.goals[0].requirement_count = 1;
    match.goals[0].requirements[0].type = REQ_CONFIGURED_SERVER;
    match.goals[0].requirements[0].subject_id = 0; // References server 0
    
    // Add a pending seeker target that references the server
    match.seeker.target_ids[0] = 0;
    match.seeker.target_count = 1;
    
    // Set up seeker belief and last_probe_tick for server 0
    match.seeker.belief[0] = 1;
    match.seeker.last_probe_tick[0] = 5;
    
    // Remove the server
    outcome = applyRemoveServer(match, 0, 0, 0xFF);
    REQUIRE(outcome == RuleOutcome::ACCEPTED);
    
    // Verify that references were invalidated:
    // - Goal requirement should be cancelled (type changed to REQ_CUSTOM_RULE)
    REQUIRE(match.goals[0].requirements[0].type == REQ_CUSTOM_RULE);
    
    // - Pending seeker target should be removed
    REQUIRE(match.seeker.target_count == 0);
    
    // - Seeker belief and last_probe_tick should be cleared
    REQUIRE(match.seeker.belief[0] == 0);
    REQUIRE(match.seeker.last_probe_tick[0] == 0);
}

TEST_CASE("applyRemoveServer handles server with occupant correctly", "[loa]") {
    MatchState match = {};
    
    // Set up a hider with LOA
    match.hiders[0].resources[0] = 2; // LOA resource at index 0
    
    // Add a server first
    RuleOutcome outcome = applyAddServer(match, 0);
    REQUIRE(outcome == RuleOutcome::ACCEPTED);
    
    // Set up a hider occupying the server
    match.hiders[1].active = 1;
    match.hiders[1].server_id = 0;
    
    // Remove the server with relocation target
    outcome = applyRemoveServer(match, 0, 0, 1); // Relocate to server 1 (which is invalid)
    REQUIRE(outcome == RuleOutcome::REJECTED_RELOCATION_FAILED);
    
    // Try valid relocation
    match.servers[1].active = 1;
    outcome = applyRemoveServer(match, 0, 0, 1);
    REQUIRE(outcome == RuleOutcome::ACCEPTED);
    
    // Verify hider was relocated
    REQUIRE(match.hiders[1].server_id == 0); // Hider should now be in server 0 (the old one)
}