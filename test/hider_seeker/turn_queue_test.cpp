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

TEST_CASE("queueMove with 0 AP queues CONFIGURE_SERVER", "[turn]") {
    MatchState state{};
    
    // Initialize a basic state
    state.schema_version = 2;
    state.phase = PHASE_ACTIVE;
    state.hider_count = 1;
    state.server_count = 2;
    
    // Set up hider with no AP (should not be immediately affordable)
    state.hiders[0].active = true;
    state.hiders[0].status = HIDER_STATUS_ACTIVE;
    state.hiders[0].action_points = 0; // No AP
    
    // Create move request for CONFIGURE_SERVER
    MoveRequest move{};
    move.move_id = 12345;
    move.hider_id = 0;
    move.action_type = static_cast<uint8_t>(TickActionKind::HIDER_ACTION);
    move.server_id = NO_SERVER_ID;
    move.target_id = NO_SERVER_ID;
    move.server_mode = 0; // Some mode
    
    StubPolicy policy{};
    
    // Test queueMove - should be queued
    TurnResult result = queueMove(state, move, policy);
    
    REQUIRE(result.status == MoveStatus::QUEUED);
    REQUIRE(result.reason == TurnReason::NONE);
    REQUIRE(state.pending_count == 1);
    REQUIRE(state.pending[0].move_id == move.move_id);
    REQUIRE(state.pending[0].hider_id == move.hider_id);
    REQUIRE(state.pending[0].action_type == move.action_type);
}

TEST_CASE("queueMove with duplicate move ID rejected", "[turn]") {
    MatchState state{};
    
    // Initialize a basic state
    state.schema_version = 2;
    state.phase = PHASE_ACTIVE;
    state.hider_count = 1;
    state.server_count = 2;
    
    // Set up hider with no AP (should not be immediately affordable)
    state.hiders[0].active = true;
    state.hiders[0].status = HIDER_STATUS_ACTIVE;
    state.hiders[0].action_points = 0; // No AP
    
    // Add a move first
    MoveRequest move1{};
    move1.move_id = 12345;
    move1.hider_id = 0;
    move1.action_type = static_cast<uint8_t>(TickActionKind::HIDER_ACTION);
    move1.server_id = NO_SERVER_ID;
    move1.target_id = NO_SERVER_ID;
    move1.server_mode = 0; // Some mode
    
    StubPolicy policy{};
    
    queueMove(state, move1, policy); // Queue the first move
    
    // Try to queue another move with same ID
    MoveRequest move2{};
    move2.move_id = 12345; // Same ID
    move2.hider_id = 0;
    move2.action_type = static_cast<uint8_t>(TickActionKind::HIDER_ACTION);
    move2.server_id = NO_SERVER_ID;
    move2.target_id = NO_SERVER_ID;
    move2.server_mode = 1; // Different mode
    
    TurnResult result = queueMove(state, move2, policy);
    
    REQUIRE(result.status == MoveStatus::REJECTED);
    REQUIRE(result.reason == TurnReason::DUPLICATE_MOVE_ID);
    REQUIRE(state.pending_count == 1); // Should still only have one move
}

TEST_CASE("queueMove with QUEUE_FULL rejected", "[turn]") {
    MatchState state{};
    
    // Initialize a basic state
    state.schema_version = 2;
    state.phase = PHASE_ACTIVE;
    state.hider_count = 1;
    state.server_count = 2;
    
    // Set up hider with no AP (should not be immediately affordable)
    state.hiders[0].active = true;
    state.hiders[0].status = HIDER_STATUS_ACTIVE;
    state.hiders[0].action_points = 0; // No AP
    
    StubPolicy policy{};
    
    // Fill the queue to MAX_PENDING_MOVES
    for (int i = 0; i < MAX_PENDING_MOVES; ++i) {
        MoveRequest move{};
        move.move_id = 1000 + i;
        move.hider_id = 0;
        move.action_type = static_cast<uint8_t>(TickActionKind::HIDER_ACTION);
        move.server_id = NO_SERVER_ID;
        move.target_id = NO_SERVER_ID;
        move.server_mode = 0; // Some mode
        
        queueMove(state, move, policy); // Queue moves
    }
    
    // Try to add one more - should be rejected with QUEUE_FULL
    MoveRequest move{};
    move.move_id = 9999;
    move.hider_id = 0;
    move.action_type = static_cast<uint8_t>(TickActionKind::HIDER_ACTION);
    move.server_id = NO_SERVER_ID;
    move.target_id = NO_SERVER_ID;
    move.server_mode = 0; // Some mode
    
    TurnResult result = queueMove(state, move, policy);
    
    REQUIRE(result.status == MoveStatus::REJECTED);
    REQUIRE(result.reason == TurnReason::QUEUE_FULL);
    REQUIRE(state.pending_count == MAX_PENDING_MOVES); // Should still be full
}

TEST_CASE("queueMove with HIDER_QUEUE_LIMIT rejected", "[turn]") {
    MatchState state{};
    
    // Initialize a basic state
    state.schema_version = 2;
    state.phase = PHASE_ACTIVE;
    state.hider_count = 1;
    state.server_count = 2;
    
    // Set up hider with no AP (should not be immediately affordable)
    state.hiders[0].active = true;
    state.hiders[0].status = HIDER_STATUS_ACTIVE;
    state.hiders[0].action_points = 0; // No AP
    
    StubPolicy policy{};
    
    // Fill the queue for this hider to PLACEHOLDER_MAX_PENDING_PER_HIDER
    for (int i = 0; i < PLACEHOLDER_MAX_PENDING_PER_HIDER; ++i) {
        MoveRequest move{};
        move.move_id = 1000 + i;
        move.hider_id = 0;
        move.action_type = static_cast<uint8_t>(TickActionKind::HIDER_ACTION);
        move.server_id = NO_SERVER_ID;
        move.target_id = NO_SERVER_ID;
        move.server_mode = 0; // Some mode
        
        queueMove(state, move, policy); // Queue moves
    }
    
    // Try to add one more for the same hider - should be rejected with HIDER_QUEUE_LIMIT
    MoveRequest move{};
    move.move_id = 9999;
    move.hider_id = 0;
    move.action_type = static_cast<uint8_t>(TickActionKind::HIDER_ACTION);
    move.server_id = NO_SERVER_ID;
    move.target_id = NO_SERVER_ID;
    move.server_mode = 0; // Some mode
    
    TurnResult result = queueMove(state, move, policy);
    
    REQUIRE(result.status == MoveStatus::REJECTED);
    REQUIRE(result.reason == TurnReason::HIDER_QUEUE_LIMIT);
    REQUIRE(state.pending_count == PLACEHOLDER_MAX_PENDING_PER_HIDER); // Should still be at limit
}

TEST_CASE("cancelPendingMove removes only caller's own move", "[turn]") {
    MatchState state{};
    
    // Initialize a basic state
    state.schema_version = 2;
    state.phase = PHASE_ACTIVE;
    state.hider_count = 2;
    state.server_count = 2;
    
    // Set up hiders with no AP (should not be immediately affordable)
    state.hiders[0].active = true;
    state.hiders[0].status = HIDER_STATUS_ACTIVE;
    state.hiders[0].action_points = 0; // No AP
    
    state.hiders[1].active = true;
    state.hiders[1].status = HIDER_STATUS_ACTIVE;
    state.hiders[1].action_points = 0; // No AP
    
    StubPolicy policy{};
    
    // Add moves from both hiders
    MoveRequest move1{};
    move1.move_id = 1001;
    move1.hider_id = 0;
    move1.action_type = static_cast<uint8_t>(TickActionKind::HIDER_ACTION);
    move1.server_id = NO_SERVER_ID;
    move1.target_id = NO_SERVER_ID;
    move1.server_mode = 0; // Some mode
    
    MoveRequest move2{};
    move2.move_id = 2002;
    move2.hider_id = 1;
    move2.action_type = static_cast<uint8_t>(TickActionKind::HIDER_ACTION);
    move2.server_id = NO_SERVER_ID;
    move2.target_id = NO_SERVER_ID;
    move2.server_mode = 0; // Some mode
    
    queueMove(state, move1, policy); // Queue first hider's move
    queueMove(state, move2, policy); // Queue second hider's move
    
    REQUIRE(state.pending_count == 2);
    
    // Cancel the first hider's move - should only cancel that one
    TurnResult result = cancelPendingMove(state, 0, 1001);
    
    REQUIRE(result.status == MoveStatus::CANCELLED);
    REQUIRE(result.reason == TurnReason::NONE);
    REQUIRE(state.pending_count == 1); // Should have one move left
    
    // The remaining move should be from hider 1
    REQUIRE(state.pending[0].hider_id == 1);
    REQUIRE(state.pending[0].move_id == 2002);
}

TEST_CASE("cancelPendingMove returns NOT_FOUND for wrong hider", "[turn]") {
    MatchState state{};
    
    // Initialize a basic state
    state.schema_version = 2;
    state.phase = PHASE_ACTIVE;
    state.hider_count = 2;
    state.server_count = 2;
    
    // Set up hiders with no AP (should not be immediately affordable)
    state.hiders[0].active = true;
    state.hiders[0].status = HIDER_STATUS_ACTIVE;
    state.hiders[0].action_points = 0; // No AP
    
    state.hiders[1].active = true;
    state.hiders[1].status = HIDER_STATUS_ACTIVE;
    state.hiders[1].action_points = 0; // No AP
    
    StubPolicy policy{};
    
    // Add a move from hider 0
    MoveRequest move1{};
    move1.move_id = 1001;
    move1.hider_id = 0;
    move1.action_type = static_cast<uint8_t>(TickActionKind::HIDER_ACTION);
    move1.server_id = NO_SERVER_ID;
    move1.target_id = NO_SERVER_ID;
    move1.server_mode = 0; // Some mode
    
    queueMove(state, move1, policy); // Queue hider's move
    
    REQUIRE(state.pending_count == 1);
    
    // Try to cancel hider 1's move when only hider 0 has a move - should return NOT_FOUND
    TurnResult result = cancelPendingMove(state, 1, 1001);
    
    REQUIRE(result.status == MoveStatus::REJECTED);
    REQUIRE(result.reason == TurnReason::NOT_FOUND);
    REQUIRE(state.pending_count == 1); // Should still have one move
}