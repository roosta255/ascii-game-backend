#include <catch2/catch_test_macros.hpp>
#include "../../../src/hider_seeker/ruleset/goal_rules.hpp"
#include "../../../src/hider_seeker/domain/state.hpp"
#include "../../../src/hider_seeker/ruleset/action_rules.hpp"
#include "../../../src/hider_seeker/ruleset/match_end.hpp"
#include "../../../src/hider_seeker/ruleset/rule_types.hpp"
#include "../../../src/hider_seeker/ruleset/server_metrics.hpp"
#include "../../../src/hider_seeker/ruleset/loa_rules.hpp"

using namespace hider_seeker;

TEST_CASE("GOAL-001: requirementMet behavior", "[goal_rules]") {
    MatchState state{};
    
    // Test REQ_ACTIVE_SERVER_COUNT
    // Initially no servers are active
    Requirement reqActiveCount{REQ_ACTIVE_SERVER_COUNT, 0, 0, 3};
    REQUIRE(!requirementMet(state, reqActiveCount));
    
    // Activate servers 0 and 1
    state.servers[0].active = 1;
    state.servers[1].active = 1;
    REQUIRE(!requirementMet(state, reqActiveCount));
    
    // Activate server 2
    state.servers[2].active = 1;
    REQUIRE(requirementMet(state, reqActiveCount));
    
    // Test REQ_SERVER_METRIC_THRESHOLD with CPU metric
    Requirement reqMetric{REQ_SERVER_METRIC_THRESHOLD, 0, SERVER_METRIC_CPU, 40};
    REQUIRE(!requirementMet(state, reqMetric)); // Server 0 is active but no metric set
    
    // Set server 0 CPU to 40 (should meet requirement)
    state.servers[0].current.cpu = 40;
    REQUIRE(requirementMet(state, reqMetric));
    
    // Set server 0 CPU to 50 (should still meet requirement)
    state.servers[0].current.cpu = 50;
    REQUIRE(requirementMet(state, reqMetric));
    
    // Set server 0 CPU to 39 (should not meet requirement)
    state.servers[0].current.cpu = 39;
    REQUIRE(!requirementMet(state, reqMetric));
    
    // Test with inactive server
    state.servers[0].active = 0; // Make server 0 inactive
    REQUIRE(!requirementMet(state, reqMetric));
    
    // Test with invalid server ID
    Requirement reqInvalid{REQ_SERVER_METRIC_THRESHOLD, MAX_SERVERS, SERVER_METRIC_CPU, 40};
    REQUIRE(!requirementMet(state, reqInvalid));
}

TEST_CASE("GOAL-001: goalRequirementsMet behavior", "[goal_rules]") {
    MatchState state{};
    
    // Test with requirement_count = 0
    GoalState goalNoReq{};
    goalNoReq.requirement_count = 0;
    REQUIRE(!goalRequirementsMet(state, goalNoReq));
    
    // Test with a valid requirement (REQ_RESOURCE_MIN - should always return false)
    GoalState goalWithReq{};
    goalWithReq.requirement_count = 1;
    goalWithReq.requirements[0].type = REQ_RESOURCE_MIN;
    goalWithReq.requirements[0].subject_id = 0;
    goalWithReq.requirements[0].amount = 0;
    goalWithReq.requirements[0].threshold = 10;
    REQUIRE(!goalRequirementsMet(state, goalWithReq));
}

TEST_CASE("GOAL-002: seedPlaceholderGoals creates correct goals", "[goal_rules]") {
    MatchState state{};
    
    // Seed the placeholder goals
    seedPlaceholderGoals(state);
    
    // Check that slots 0-3 have the correct values
    REQUIRE(state.goals[0].active == 1);
    REQUIRE(state.goals[0].goal_id == 0);
    REQUIRE(state.goals[0].status == GOAL_STATUS_OPEN);
    REQUIRE(state.goals[0].reward_type == REWARD);
    REQUIRE(state.goals[0].reward_amount == 1);
    REQUIRE(state.goals[0].requirement_count == 1);
    REQUIRE(state.goals[0].completion_action == 0);
    REQUIRE(state.goals[0].deadline_tick == 0);
    REQUIRE(state.goals[0].requirements[0].type == REQ_SERVER_METRIC_THRESHOLD);
    REQUIRE(state.goals[0].requirements[0].subject_id == 0);
    REQUIRE(state.goals[0].requirements[0].amount == SERVER_METRIC_CPU);
    REQUIRE(state.goals[0].requirements[0].threshold == 40);
    
    REQUIRE(state.goals[1].active == 1);
    REQUIRE(state.goals[1].goal_id == 1);
    REQUIRE(state.goals[1].status == GOAL_STATUS_OPEN);
    REQUIRE(state.goals[1].reward_type == REWARD);
    REQUIRE(state.goals[1].reward_amount == 1);
    REQUIRE(state.goals[1].requirement_count == 1);
    REQUIRE(state.goals[1].completion_action == 0);
    REQUIRE(state.goals[1].deadline_tick == 0);
    REQUIRE(state.goals[1].requirements[0].type == REQ_SERVER_METRIC_THRESHOLD);
    REQUIRE(state.goals[1].requirements[0].subject_id == 0);
    REQUIRE(state.goals[1].requirements[0].amount == SERVER_METRIC_NETWORK);
    REQUIRE(state.goals[1].requirements[0].threshold == 20);
    
    REQUIRE(state.goals[2].active == 1);
    REQUIRE(state.goals[2].goal_id == 2);
    REQUIRE(state.goals[2].status == GOAL_STATUS_OPEN);
    REQUIRE(state.goals[2].reward_type == REWARD);
    REQUIRE(state.goals[2].reward_amount == 1);
    REQUIRE(state.goals[2].requirement_count == 1);
    REQUIRE(state.goals[2].completion_action == 0);
    REQUIRE(state.goals[2].deadline_tick == 0);
    REQUIRE(state.goals[2].requirements[0].type == REQ_ACTIVE_SERVER_COUNT);
    REQUIRE(state.goals[2].requirements[0].subject_id == 0);
    REQUIRE(state.goals[2].requirements[0].amount == 0);
    REQUIRE(state.goals[2].requirements[0].threshold == 3);
    
    REQUIRE(state.goals[3].active == 1);
    REQUIRE(state.goals[3].goal_id == 3);
    REQUIRE(state.goals[3].status == GOAL_STATUS_OPEN);
    REQUIRE(state.goals[3].reward_type == REWARD);
    REQUIRE(state.goals[3].reward_amount == 1);
    REQUIRE(state.goals[3].requirement_count == 1);
    REQUIRE(state.goals[3].completion_action == 0);
    REQUIRE(state.goals[3].deadline_tick == 0);
    REQUIRE(state.goals[3].requirements[0].type == REQ_SERVER_METRIC_THRESHOLD);
    REQUIRE(state.goals[3].requirements[0].subject_id == 0);
    REQUIRE(state.goals[3].requirements[0].amount == SERVER_METRIC_CPU);
    REQUIRE(state.goals[3].requirements[0].threshold == 80);
    
    // Check that slots 4-15 are zeroed
    for (int i = 4; i < MAX_GOALS; ++i) {
        REQUIRE(state.goals[i].active == 0);
    }
    
    // Test idempotency - call again and verify same results
    MatchState state2{};
    seedPlaceholderGoals(state2);
    
    for (int i = 0; i < MAX_GOALS; ++i) {
        REQUIRE(state.goals[i].active == state2.goals[i].active);
        REQUIRE(state.goals[i].goal_id == state2.goals[i].goal_id);
        REQUIRE(state.goals[i].status == state2.goals[i].status);
        REQUIRE(state.goals[i].reward_type == state2.goals[i].reward_type);
        REQUIRE(state.goals[i].reward_amount == state2.goals[i].reward_amount);
        REQUIRE(state.goals[i].requirement_count == state2.goals[i].requirement_count);
        REQUIRE(state.goals[i].completion_action == state2.goals[i].completion_action);
        REQUIRE(state.goals[i].deadline_tick == state2.goals[i].deadline_tick);
    }
}

TEST_CASE("GOAL-003: PAY_GOAL action completes goals", "[goal_rules]") {
    MatchState state{};
    
    // Set up servers 0 and 1 as active
    state.servers[0].active = 1;
    state.servers[1].active = 1;
    
    // Set up hider 0 on server 0 with 10 AP
    state.hiders[0].active = 1;
    state.hiders[0].server_id = 0;
    state.hiders[0].action_points = 10;
    
    // Seed placeholder goals
    seedPlaceholderGoals(state);
    
    // Test: with cpu 50 and network 0, PAY_GOAL should complete goal 0 only
    state.servers[0].current.cpu = 50;
    state.servers[0].current.network = 0;
    
    RuleOutcome outcome = applyHiderAction(state, 0, HiderActionMode::PAY_GOAL);
    REQUIRE(outcome == RuleOutcome::ACCEPTED);
    REQUIRE(state.hiders[0].action_points == 0); // AP spent
    REQUIRE(state.goals[0].status == GOAL_STATUS_COMPLETED); // Goal 0 completed
    REQUIRE(state.goals[1].status == GOAL_STATUS_OPEN); // Goal 1 not completed
    REQUIRE(state.goals[2].status == GOAL_STATUS_OPEN); // Goal 2 not completed
    REQUIRE(state.goals[3].status == GOAL_STATUS_OPEN); // Goal 3 not completed
    
    // Test: with another 10 AP (no other changes), should complete nothing more
    state.hiders[0].action_points = 10;
    outcome = applyHiderAction(state, 0, HiderActionMode::PAY_GOAL);
    REQUIRE(outcome == RuleOutcome::ACCEPTED);
    REQUIRE(state.hiders[0].action_points == 0); // AP spent
    REQUIRE(state.goals[0].status == GOAL_STATUS_COMPLETED); // Goal 0 still completed
    REQUIRE(state.goals[1].status == GOAL_STATUS_OPEN); // Goal 1 not completed
    REQUIRE(state.goals[2].status == GOAL_STATUS_OPEN); // Goal 2 not completed
    REQUIRE(state.goals[3].status == GOAL_STATUS_OPEN); // Goal 3 not completed
    
    // Test: with cpu 90, network 30 and server 2 active, four PAY_GOALs should complete goals in order
    state.servers[2].active = 1;
    state.servers[0].current.cpu = 90;
    state.servers[0].current.network = 30;
    
    // Complete goal 0 (CPU threshold 40)
    state.hiders[0].action_points = 10;
    outcome = applyHiderAction(state, 0, HiderActionMode::PAY_GOAL);
    REQUIRE(outcome == RuleOutcome::ACCEPTED);
    REQUIRE(state.hiders[0].action_points == 0);
    REQUIRE(state.goals[0].status == GOAL_STATUS_COMPLETED);
    
    // Complete goal 1 (Network threshold 20)
    state.hiders[0].action_points = 10;
    outcome = applyHiderAction(state, 0, HiderActionMode::PAY_GOAL);
    REQUIRE(outcome == RuleOutcome::ACCEPTED);
    REQUIRE(state.hiders[0].action_points == 0);
    REQUIRE(state.goals[1].status == GOAL_STATUS_COMPLETED);
    
    // Complete goal 2 (3 servers active)
    state.hiders[0].action_points = 10;
    outcome = applyHiderAction(state, 0, HiderActionMode::PAY_GOAL);
    REQUIRE(outcome == RuleOutcome::ACCEPTED);
    REQUIRE(state.hiders[0].action_points == 0);
    REQUIRE(state.goals[2].status == GOAL_STATUS_COMPLETED);
    
    // Complete goal 3 (CPU threshold 80)
    state.hiders[0].action_points = 10;
    outcome = applyHiderAction(state, 0, HiderActionMode::PAY_GOAL);
    REQUIRE(outcome == RuleOutcome::ACCEPTED);
    REQUIRE(state.hiders[0].action_points == 0);
    REQUIRE(state.goals[3].status == GOAL_STATUS_COMPLETED);
    
    // Test: with only 9 AP, should be rejected
    state.hiders[0].action_points = 9;
    outcome = applyHiderAction(state, 0, HiderActionMode::PAY_GOAL);
    REQUIRE(outcome == RuleOutcome::REJECTED_INSUFFICIENT_RESOURCE);
    REQUIRE(state.hiders[0].action_points == 9); // No AP spent
    REQUIRE(state.goals[0].status == GOAL_STATUS_COMPLETED); // No change
    REQUIRE(state.goals[1].status == GOAL_STATUS_COMPLETED); // No change
    REQUIRE(state.goals[2].status == GOAL_STATUS_COMPLETED); // No change
    REQUIRE(state.goals[3].status == GOAL_STATUS_COMPLETED); // No change
}

TEST_CASE("GOAL-004: teamReward calculation", "[goal_rules]") {
    MatchState state{};
    
    // Seed goals and check initial reward is 0
    seedPlaceholderGoals(state);
    REQUIRE(teamReward(state) == 0);
    
    // Complete goals 0 and 1, check reward is 2
    state.goals[0].status = GOAL_STATUS_COMPLETED;
    state.goals[1].status = GOAL_STATUS_COMPLETED;
    REQUIRE(teamReward(state) == 2);
    
    // Add a goal with status GOAL_STATUS_CANCELLED_SERVER_REMOVED (should not count)
    state.goals[4].active = 1;
    state.goals[4].status = GOAL_STATUS_CANCELLED_SERVER_REMOVED;
    state.goals[4].reward_amount = 5;
    REQUIRE(teamReward(state) == 2); // Still 2, because cancelled goal doesn't count
    
    // Add a goal with active = 0 (should not count)
    state.goals[5].active = 0;
    state.goals[5].status = GOAL_STATUS_COMPLETED;
    state.goals[5].reward_amount = 3;
    REQUIRE(teamReward(state) == 2); // Still 2, because inactive goal doesn't count
    
    // Add a valid completed goal
    state.goals[6].active = 1;
    state.goals[6].status = GOAL_STATUS_COMPLETED;
    state.goals[6].reward_amount = 4;
    REQUIRE(teamReward(state) == 6); // Now 2 + 4 = 6
}

TEST_CASE("GOAL-005: hider win condition in checkMatchEnd", "[goal_rules]") {
    MatchState state{};
    
    // Set up match state for win condition
    state.phase = PHASE_ACTIVE;
    state.winner = static_cast<uint8_t>(MatchWinner::NONE);
    
    // Seed placeholder goals
    seedPlaceholderGoals(state);
    
    // Set up seeker with connection tokens
    state.seeker.connection_tokens = 5;
    
    // Set up hider 0 on server 0
    state.hiders[0].active = 1;
    state.hiders[0].server_id = 0;
    
    // Set up hider connection pool
    state.hider_connection_pool = 3;
    
    // Complete goals 0, 1 and 2 (total reward = 3 which meets target of 3)
    state.goals[0].status = GOAL_STATUS_COMPLETED;
    state.goals[1].status = GOAL_STATUS_COMPLETED;
    state.goals[2].status = GOAL_STATUS_COMPLETED;
    
    checkMatchEnd(state);
    
    REQUIRE(state.phase == PHASE_COMPLETE);
    REQUIRE(state.winner == static_cast<uint8_t>(MatchWinner::HIDERS));
    
    // Test: goals 0 and 1 completed (total reward = 2, less than target of 3)
    state.phase = PHASE_ACTIVE;
    state.winner = static_cast<uint8_t>(MatchWinner::NONE);
    state.goals[0].status = GOAL_STATUS_COMPLETED;
    state.goals[1].status = GOAL_STATUS_COMPLETED;
    state.goals[2].status = GOAL_STATUS_OPEN; // Goal 2 not completed
    
    checkMatchEnd(state);
    
    REQUIRE(state.phase == PHASE_ACTIVE); // Should remain active
    REQUIRE(state.winner == static_cast<uint8_t>(MatchWinner::NONE)); // Should remain none
    
    // Test: goals 0, 1, 2 completed AND hiders lose condition (hider_connection_pool = 0 and hider on server)
    state.phase = PHASE_ACTIVE;
    state.winner = static_cast<uint8_t>(MatchWinner::NONE);
    state.hider_connection_pool = 0; // This makes hiders lose
    state.goals[0].status = GOAL_STATUS_COMPLETED;
    state.goals[1].status = GOAL_STATUS_COMPLETED;
    state.goals[2].status = GOAL_STATUS_COMPLETED;
    
    checkMatchEnd(state);
    
    REQUIRE(state.phase == PHASE_COMPLETE); // Should complete (win takes precedence)
    REQUIRE(state.winner == static_cast<uint8_t>(MatchWinner::HIDERS)); // Should be hider win
    
    // Test: phase = PHASE_LOBBY with goals completed (should not change anything)
    state.phase = PHASE_LOBBY;
    state.winner = static_cast<uint8_t>(MatchWinner::NONE);
    
    checkMatchEnd(state);
    
    REQUIRE(state.phase == PHASE_LOBBY); // Should remain unchanged
    REQUIRE(state.winner == static_cast<uint8_t>(MatchWinner::NONE)); // Should remain unchanged
}

TEST_CASE("GOAL-001: constants are defined correctly", "[goal_rules]") {
    REQUIRE(GOAL_STATUS_OPEN == 0);
    REQUIRE(GOAL_STATUS_COMPLETED == 1);
    REQUIRE(PLACEHOLDER_HIDER_REWARD_TARGET == 3);
}