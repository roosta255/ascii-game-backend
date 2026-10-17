#include <catch2/catch_test_macros.hpp>
#include "../../../src/hider_seeker/turn/turn_engine.hpp"
#include "../../../src/hider_seeker/domain/state.hpp"
#include "../../../src/hider_seeker/ruleset/rule_types.hpp"
#include "../../../src/hider_seeker/ruleset/action_rules.hpp"
#include "../../../src/hider_seeker/ruleset/target_ranking_interface.hpp"
#include "../../../src/hider_seeker/ruleset/connection_rules.hpp"

using namespace hider_seeker;

// Simple mock policy for testing
struct MockTargetRankingPolicy : public iTargetRankingPolicy {
    void rankTargets(const MatchState&, Array<ProbeCandidate, MAX_SERVERS>&) const override {}
};

// This test validates that applyHiderAction rejects actions when server_id == NO_SERVER_ID
// and that the turn engine treats REJECTED_NO_CONNECTIONS as an invalid move.

TEST_CASE("applyHiderAction rejects hider actions when hider is not connected", "[turn_engine]") {
    MatchState state{};
    state.hiders[0].active = true;
    state.hiders[0].server_id = NO_SERVER_ID; // Not connected
    state.hiders[0].action_points = 10;
    
    // Test that BOOST_CPU fails when hider is not connected (server_id == NO_SERVER_ID)
    RuleOutcome outcome = applyHiderAction(state, 0, HiderActionMode::BOOST_CPU);
    REQUIRE(outcome == RuleOutcome::REJECTED_INVALID_TARGET);
    
    // Verify that no AP was spent
    REQUIRE(state.hiders[0].action_points == 10);
}

TEST_CASE("turn_engine_treats_REJECTED_NO_CONNECTIONS_as_invalid_move", "[turn_engine]") {
    MatchState state{};
    state.phase = PHASE_ACTIVE;
    state.hiders[0].active = true;
    state.hiders[0].server_id = NO_SERVER_ID; // Not connected
    state.hiders[0].action_points = 10;
    
    MoveRequest move{};
    move.move_id = 1;
    move.hider_id = 0;
    move.action_type = static_cast<uint8_t>(TickActionKind::HIDER_ACTION);
    move.server_mode = static_cast<uint8_t>(HiderActionMode::BOOST_CPU);
    
    // Test that a move with BOOST_CPU fails immediately when hider is not connected
    MockTargetRankingPolicy policy{};
    TurnResult result = playMove(state, move, policy);
    
    REQUIRE(result.status == MoveStatus::REJECTED);
    REQUIRE(result.ruleOutcome == RuleOutcome::REJECTED_INVALID_TARGET);
}

TEST_CASE("turn_engine_treats_REJECTED_NO_CONNECTIONS_as_invalid_move_in_queue", "[turn_engine]") {
    MatchState state{};
    state.phase = PHASE_ACTIVE;
    state.hiders[0].active = true;
    state.hiders[0].server_id = NO_SERVER_ID; // Not connected
    state.hiders[0].action_points = 10;
    
    MoveRequest move{};
    move.move_id = 1;
    move.hider_id = 0;
    move.action_type = static_cast<uint8_t>(TickActionKind::HIDER_ACTION);
    move.server_mode = static_cast<uint8_t>(HiderActionMode::BOOST_CPU);
    
    // Test that a queued move with BOOST_CPU fails immediately when hider is not connected
    MockTargetRankingPolicy policy{};
    TurnResult result = queueMove(state, move, policy);
    
    REQUIRE(result.status == MoveStatus::REJECTED);
    REQUIRE(result.ruleOutcome == RuleOutcome::REJECTED_INVALID_TARGET);
}

TEST_CASE("applyConnect_and_applyDisconnect_work_correctly", "[connection]") {
    MatchState state{};
    state.hiders[0].active = true;
    state.hiders[0].server_id = NO_SERVER_ID; // Not connected
    state.hiders[0].action_points = 10;
    state.servers[0].active = true;
    state.hider_connection_pool = 1;
    
    // Test applyConnect
    RuleOutcome connectOutcome = applyConnect(state, 0, 0);
    REQUIRE(connectOutcome == RuleOutcome::ACCEPTED);
    REQUIRE(state.hiders[0].server_id == 0);
    REQUIRE(state.hider_connection_pool == 0);
    
    // Test applyDisconnect
    RuleOutcome disconnectOutcome = applyDisconnect(state, 0);
    REQUIRE(disconnectOutcome == RuleOutcome::ACCEPTED);
    REQUIRE(state.hiders[0].server_id == NO_SERVER_ID);
    REQUIRE(state.hider_connection_pool == 1);
}