#include <catch2/catch_test_macros.hpp>
#include "action_rules.hpp"
#include "state.hpp"

using namespace hider_seeker;

TEST_CASE("applyHiderAction uses backend table for cost/effect and deducts only on acceptance", "[action]") {
    MatchState match = {};
    
    // Initialize a hider with 10 AP
    match.hiders[0].action_points = 10;
    
    // Apply an action - should succeed
    RuleOutcome outcome = applyHiderAction(match, 0, HiderActionMode::CONFIGURE_SERVER);
    REQUIRE(outcome == RuleOutcome::ACCEPTED);
    
    // Check that AP was deducted (placeholder value of 5)
    REQUIRE(match.hiders[0].action_points == 5);
}

TEST_CASE("applyHiderAction does not deduct AP on rejection", "[action]") {
    MatchState match = {};
    
    // Initialize a hider with 2 AP (less than the placeholder cost of 5)
    match.hiders[0].action_points = 2;
    
    // Apply an action - should fail due to insufficient AP
    RuleOutcome outcome = applyHiderAction(match, 0, HiderActionMode::CONFIGURE_SERVER);
    REQUIRE(outcome == RuleOutcome::REJECTED_INSUFFICIENT_RESOURCE);
    
    // Check that AP was not deducted
    REQUIRE(match.hiders[0].action_points == 2);
}

TEST_CASE("applyHiderAction always takes cost/effect from backend table", "[action]") {
    MatchState match = {};
    
    // Initialize a hider with sufficient AP for all actions
    match.hiders[0].action_points = 20;
    
    // Test different action modes - all should use backend table values
    RuleOutcome outcome1 = applyHiderAction(match, 0, HiderActionMode::CONFIGURE_SERVER);
    REQUIRE(outcome1 == RuleOutcome::ACCEPTED);
    
    RuleOutcome outcome2 = applyHiderAction(match, 0, HiderActionMode::SUPPRESS_ERROR);
    REQUIRE(outcome2 == RuleOutcome::ACCEPTED);
    
    RuleOutcome outcome3 = applyHiderAction(match, 0, HiderActionMode::BOOST_CPU);
    REQUIRE(outcome3 == RuleOutcome::ACCEPTED);
    
    // Check that AP was deducted for each action (using placeholder values)
    // CONFIGURE_SERVER: 5, SUPPRESS_ERROR: 3, BOOST_CPU: 2
    REQUIRE(match.hiders[0].action_points == 20 - 5 - 3 - 2); // Should be 10
}

TEST_CASE("applyHiderAction applies the table-defined effect to the hider's occupied server", "[action]") {
    MatchState match = {};
    match.hiders[0].action_points = 20;
    match.hiders[0].server_id = 2;
    match.servers[2].active = 1;
    match.servers[2].current.cpu = 50;
    match.servers[2].current.errors = 4;
    match.servers[2].current.network = 250;
    const uint8_t serverCountBefore = match.server_count;

    // BOOST_CPU: placeholder +15 cpu
    REQUIRE(applyHiderAction(match, 0, HiderActionMode::BOOST_CPU) == RuleOutcome::ACCEPTED);
    REQUIRE(match.servers[2].current.cpu == 65);

    // SUPPRESS_ERROR: placeholder -10 errors, clamped at 0
    REQUIRE(applyHiderAction(match, 0, HiderActionMode::SUPPRESS_ERROR) == RuleOutcome::ACCEPTED);
    REQUIRE(match.servers[2].current.errors == 0);

    // ROTATE_ROUTE: placeholder +5 network, clamped at 255
    REQUIRE(applyHiderAction(match, 0, HiderActionMode::ROTATE_ROUTE) == RuleOutcome::ACCEPTED);
    REQUIRE(match.servers[2].current.network == 255);

    // Effects never touch structural state such as server_count
    REQUIRE(match.server_count == serverCountBefore);
}

TEST_CASE("applyHiderAction applies no effect when rejected", "[action]") {
    MatchState match = {};
    match.hiders[0].action_points = 1; // below BOOST_CPU placeholder cost of 2
    match.hiders[0].server_id = 0;
    match.servers[0].active = 1;
    match.servers[0].current.cpu = 50;

    REQUIRE(applyHiderAction(match, 0, HiderActionMode::BOOST_CPU) == RuleOutcome::REJECTED_INSUFFICIENT_RESOURCE);
    REQUIRE(match.servers[0].current.cpu == 50);
    REQUIRE(match.hiders[0].action_points == 1);
}