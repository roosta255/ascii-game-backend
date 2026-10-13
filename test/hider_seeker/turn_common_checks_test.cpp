#include <catch2/catch_test_macros.hpp>
#include "../../src/hider_seeker/domain/state.hpp"
#include "../../src/hider_seeker/turn/turn_engine.hpp"

using namespace hider_seeker;

// Stub policy that does nothing
struct StubPolicy : public iTargetRankingPolicy {
    void rankTargets(const MatchState& state, Array<ProbeCandidate, MAX_SERVERS>& candidates) override {
        // Do nothing
    }
};

TEST_CASE("Common checks MATCH_NOT_ACTIVE", "[turn]") {
    MatchState state{};
    
    // Initialize a basic state but with PHASE_LOBBY (not active)
    state.schema_version = 2;
    state.phase = PHASE_LOBBY; // Not active
    state.hider_count = 1;
    state.server_count = 2;
    
    // Set up hider
    state.hiders[0].active = true;
    state.hiders[0].status = HIDER_STATUS_ACTIVE;
    state.hiders[0].action_points = 5; // Enough AP
    
    // Set up servers
    state.servers[0].active = true;
    state.servers[0].owner_hider_id = NO_SERVER_ID;
    
    state.servers[1].active = true;
    state.servers[1].owner_hider_id = NO_SERVER_ID;
    
    MoveRequest move{};
    move.move_id = 12345;
    move.hider_id = 0;
    move.action_type = static_cast<uint8_t>(TickActionKind::HIDER_ACTION);
    move.server_id = NO_SERVER_ID;
    move.target_id = NO_SERVER_ID;
    move.server_mode = 0; // Some mode
    
    StubPolicy policy{};
    
    // Test playMove
    TurnResult result1 = playMove(state, move, policy);
    REQUIRE(result1.status == MoveStatus::REJECTED);
    REQUIRE(result1.reason == TurnReason::MATCH_NOT_ACTIVE);
    
    // Test queueMove
    TurnResult result2 = queueMove(state, move, policy);
    REQUIRE(result2.status == MoveStatus::REJECTED);
    REQUIRE(result2.reason == TurnReason::MATCH_NOT_ACTIVE);
    
    // Test passTicks
    TurnResult result3 = passTicks(state, 1, policy);
    REQUIRE(result3.status == MoveStatus::REJECTED);
    REQUIRE(result3.reason == TurnReason::MATCH_NOT_ACTIVE);
    
    // Test cancelPendingMove
    TurnResult result4 = cancelPendingMove(state, 0, 12345);
    REQUIRE(result4.status == MoveStatus::REJECTED);
    REQUIRE(result4.reason == TurnReason::MATCH_NOT_ACTIVE);
}

TEST_CASE("Common checks HIDER_CAPTURED", "[turn]") {
    MatchState state{};
    
    // Initialize a basic state
    state.schema_version = 2;
    state.phase = PHASE_ACTIVE;
    state.hider_count = 1;
    state.server_count = 2;
    
    // Set up captured hider
    state.hiders[0].active = true;
    state.hiders[0].status = HIDER_STATUS_CAPTURED; // Captured
    state.hiders[0].action_points = 5; // Enough AP
    
    // Set up servers
    state.servers[0].active = true;
    state.servers[0].owner_hider_id = NO_SERVER_ID;
    
    state.servers[1].active = true;
    state.servers[1].owner_hider_id = NO_SERVER_ID;
    
    MoveRequest move{};
    move.move_id = 12345;
    move.hider_id = 0;
    move.action_type = static_cast<uint8_t>(TickActionKind::HIDER_ACTION);
    move.server_id = NO_SERVER_ID;
    move.target_id = NO_SERVER_ID;
    move.server_mode = 0; // Some mode
    
    StubPolicy policy{};
    
    // Test playMove
    TurnResult result1 = playMove(state, move, policy);
    REQUIRE(result1.status == MoveStatus::REJECTED);
    REQUIRE(result1.reason == TurnReason::HIDER_CAPTURED);
    
    // Test queueMove
    TurnResult result2 = queueMove(state, move, policy);
    REQUIRE(result2.status == MoveStatus::REJECTED);
    REQUIRE(result2.reason == TurnReason::HIDER_CAPTURED);
    
    // Test passTicks - should not be rejected for this check
    TurnResult result3 = passTicks(state, 1, policy);
    REQUIRE(result3.status == MoveStatus::APPLIED); // Should still work
    
    // Test cancelPendingMove - should not be rejected for this check
    TurnResult result4 = cancelPendingMove(state, 0, 12345);
    REQUIRE(result4.status == MoveStatus::REJECTED); // Will fail due to no pending moves
}

TEST_CASE("Common checks RULE_REJECTED/REJECTED_INVALID_TARGET", "[turn]") {
    MatchState state{};
    
    // Initialize a basic state
    state.schema_version = 2;
    state.phase = PHASE_ACTIVE;
    state.hider_count = 1;
    state.server_count = 2;
    
    // Set up inactive hider (should be rejected)
    state.hiders[0].active = false; // Inactive
    state.hiders[0].status = HIDER_STATUS_ACTIVE;
    state.hiders[0].action_points = 5; // Enough AP
    
    // Set up servers
    state.servers[0].active = true;
    state.servers[0].owner_hider_id = NO_SERVER_ID;
    
    state.servers[1].active = true;
    state.servers[1].owner_hider_id = NO_SERVER_ID;
    
    MoveRequest move{};
    move.move_id = 12345;
    move.hider_id = 0;
    move.action_type = static_cast<uint8_t>(TickActionKind::HIDER_ACTION);
    move.server_id = NO_SERVER_ID;
    move.target_id = NO_SERVER_ID;
    move.server_mode = 0; // Some mode
    
    StubPolicy policy{};
    
    // Test playMove
    TurnResult result1 = playMove(state, move, policy);
    REQUIRE(result1.status == MoveStatus::REJECTED);
    REQUIRE(result1.reason == TurnReason::RULE_REJECTED);
    REQUIRE(result1.ruleOutcome == RuleOutcome::REJECTED_INVALID_TARGET);
    
    // Test queueMove
    TurnResult result2 = queueMove(state, move, policy);
    REQUIRE(result2.status == MoveStatus::REJECTED);
    REQUIRE(result2.reason == TurnReason::RULE_REJECTED);
    REQUIRE(result2.ruleOutcome == RuleOutcome::REJECTED_INVALID_TARGET);
}

TEST_CASE("Common checks revision incrementing exactly once per non-REJECTED call", "[turn]") {
    MatchState state{};
    
    // Initialize a basic state
    state.schema_version = 2;
    state.phase = PHASE_ACTIVE;
    state.revision = 1000;
    state.hider_count = 1;
    state.server_count = 2;
    
    // Set up hider with enough AP
    state.hiders[0].active = true;
    state.hiders[0].status = HIDER_STATUS_ACTIVE;
    state.hiders[0].action_points = 5; // Enough AP
    
    // Set up servers
    state.servers[0].active = true;
    state.servers[0].owner_hider_id = NO_SERVER_ID;
    
    state.servers[1].active = true;
    state.servers[1].owner_hider_id = NO_SERVER_ID;
    
    MoveRequest move{};
    move.move_id = 12345;
    move.hider_id = 0;
    move.action_type = static_cast<uint8_t>(TickActionKind::HIDER_ACTION);
    move.server_id = NO_SERVER_ID;
    move.target_id = NO_SERVER_ID;
    move.server_mode = 0; // Some mode
    
    StubPolicy policy{};
    
    uint64_t original_revision = state.revision;
    
    // Test playMove - should increment revision
    TurnResult result1 = playMove(state, move, policy);
    REQUIRE(result1.status == MoveStatus::APPLIED || result1.status == MoveStatus::QUEUED);
    REQUIRE(state.revision == original_revision + 1);
    
    // Reset revision for next test
    state.revision = original_revision;
    
    // Test queueMove - should increment revision
    TurnResult result2 = queueMove(state, move, policy);
    REQUIRE(result2.status == MoveStatus::QUEUED);
    REQUIRE(state.revision == original_revision + 1);
}