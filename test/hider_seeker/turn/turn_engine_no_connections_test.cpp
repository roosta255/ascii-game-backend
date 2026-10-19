#include <catch2/catch_test_macros.hpp>
#include "../../../src/hider_seeker/turn/turn_engine.hpp"
#include "../../../src/hider_seeker/domain/state.hpp"
#include "../../../src/hider_seeker/ruleset/rule_types.hpp"
#include "../../../src/hider_seeker/ruleset/action_rules.hpp"
#include "../../../src/hider_seeker/seeker/policy_interface.hpp"

using namespace hider_seeker;

// Simple mock policy for testing
struct MockTargetRankingPolicy : public iSeekerPolicy {
    void score(const SeekerObservation&, Array<SeekerServerScores, MAX_SERVERS>&) const override {}
    std::string policyId() const override { return "test"; }
};

// CONN-009: A unit test confirms that a move rejected for lack of connections 
// does not become pending or retried.
TEST_CASE("move_rejected_for_no_connections_does_not_become_pending", "[turn_engine][conn-009]") {
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
    
    MockTargetRankingPolicy policy{};
    TurnResult result = playMove(state, move, policy);
    
    REQUIRE(result.status == MoveStatus::REJECTED);
    REQUIRE(result.ruleOutcome == RuleOutcome::REJECTED_INVALID_TARGET);
    
    // The move should not be queued/pending
    REQUIRE(state.pending_count == 0);
}

// CONN-009: Test that queueMove also rejects moves for lack of connections
TEST_CASE("queueMove_rejected_for_no_connections_does_not_become_pending", "[turn_engine][conn-009]") {
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
    
    MockTargetRankingPolicy policy{};
    TurnResult result = queueMove(state, move, policy);
    
    REQUIRE(result.status == MoveStatus::REJECTED);
    REQUIRE(result.ruleOutcome == RuleOutcome::REJECTED_INVALID_TARGET);
    
    // The move should not be queued/pending
    REQUIRE(state.pending_count == 0);
}

// CONN-009: Test that a move rejected for lack of connections is not retried
TEST_CASE("move_rejected_for_no_connections_is_not_retried", "[turn_engine][conn-009]") {
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
    
    MockTargetRankingPolicy policy{};
    TurnResult result = playMove(state, move, policy);
    
    REQUIRE(result.status == MoveStatus::REJECTED);
    REQUIRE(result.ruleOutcome == RuleOutcome::REJECTED_INVALID_TARGET);
    
    // The move should not be queued/pending
    REQUIRE(state.pending_count == 0);
}