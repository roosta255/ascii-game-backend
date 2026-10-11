#include <catch2/catch_test_macros.hpp>
#include "hider_seeker/ruleset/action_rules.hpp"
#include "hider_seeker/domain/state.hpp"

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