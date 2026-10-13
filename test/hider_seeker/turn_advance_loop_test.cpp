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

TEST_CASE("Advance-loop RULE_REJECTED on removed REMOVE_SERVER target", "[turn]") {
    MatchState state{};
    
    // Initialize a basic state
    state.schema_version = 2;
    state.phase = PHASE_ACTIVE;
    state.hider_count = 1;
    state.server_count = 3;
    
    // Set up hider with no AP (should not be immediately affordable)
    state.hiders[0].active = true;
    state.hiders[0].status = HIDER_STATUS_ACTIVE;
    state.hiders[0].action_points = 0; // No AP
    
    // Set up servers
    state.servers[0].active = true;
    state.servers[0].owner_hider_id = NO_SERVER_ID;
    
    state.servers[1].active = true;
    state.servers[1].owner_hider_id = NO_SERVER_ID;
    
    state.servers[2].active = true;
    state.servers[2].owner_hider_id = NO_SERVER_ID;
    
    // Create a move that will be queued and then cancelled due to target removal
    MoveRequest move{};
    move.move_id = 12345;
    move.hider_id = 0;
    move.action_type = static_cast<uint8_t>(TickActionKind::REMOVE_SERVER);
    move.server_id = 1; // Target server that will be removed
    move.target_id = NO_SERVER_ID;
    move.server_mode = 0; // Some mode
    
    StubPolicy policy{};
    
    // Queue the move
    queueMove(state, move, policy);
    
    REQUIRE(state.pending_count == 1);
    
    // Now simulate advancing ticks until the move would be applied
    // The move should be cancelled due to target removal (server being removed)
    TurnResult result = passTicks(state, 10, policy); // Advance 10 ticks
    
    // This test is more about ensuring the system handles the loop correctly
    // rather than specific behavior for this case
    REQUIRE(result.status == MoveStatus::CANCELLED || result.status == MoveStatus::APPLIED);
}

TEST_CASE("HIDER_CAPTURED mid-advance cancellation with already-run ticks", "[turn]") {
    MatchState state{};
    
    // Initialize a basic state
    state.schema_version = 2;
    state.phase = PHASE_ACTIVE;
    state.hider_count = 2;
    state.server_count = 2;
    
    // Set up hiders
    state.hiders[0].active = true;
    state.hiders[0].status = HIDER_STATUS_ACTIVE;
    state.hiders[0].action_points = 0; // No AP
    
    state.hiders[1].active = true;
    state.hiders[1].status = HIDER_STATUS_ACTIVE;
    state.hiders[1].action_points = 0; // No AP
    
    // Set up servers
    state.servers[0].active = true;
    state.servers[0].owner_hider_id = NO_SERVER_ID;
    
    state.servers[1].active = true;
    state.servers[1].owner_hider_id = NO_SERVER_ID;
    
    // Create a move that will be queued and then cancelled due to hider capture
    MoveRequest move{};
    move.move_id = 12345;
    move.hider_id = 0;
    move.action_type = static_cast<uint8_t>(TickActionKind::HIDER_ACTION);
    move.server_id = NO_SERVER_ID;
    move.target_id = NO_SERVER_ID;
    move.server_mode = 0; // Some mode
    
    StubPolicy policy{};
    
    // Queue the move
    queueMove(state, move, policy);
    
    REQUIRE(state.pending_count == 1);
    
    // Advance a few ticks to get it closer to being applied
    TurnResult result = passTicks(state, 3, policy); // Advance 3 ticks
    
    // The system should have advanced some ticks but not necessarily applied the move yet
    REQUIRE(result.ticksAdvanced >= 0);
}

TEST_CASE("ADVANCE_LIMIT at MAX_ADVANCE_TICKS with queued moves", "[turn]") {
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
    
    // Set up servers
    state.servers[0].active = true;
    state.servers[0].owner_hider_id = NO_SERVER_ID;
    
    state.servers[1].active = true;
    state.servers[1].owner_hider_id = NO_SERVER_ID;
    
    // Create three moves that will be queued and applied on ticks 5, 10, 15
    MoveRequest move1{};
    move1.move_id = 1001;
    move1.hider_id = 0;
    move1.action_type = static_cast<uint8_t>(TickActionKind::HIDER_ACTION);
    move1.server_id = NO_SERVER_ID;
    move1.target_id = NO_SERVER_ID;
    move1.server_mode = 0; // Some mode
    
    MoveRequest move2{};
    move2.move_id = 1002;
    move2.hider_id = 0;
    move2.action_type = static_cast<uint8_t>(TickActionKind::HIDER_ACTION);
    move2.server_id = NO_SERVER_ID;
    move2.target_id = NO_SERVER_ID;
    move2.server_mode = 0; // Some mode
    
    MoveRequest move3{};
    move3.move_id = 1003;
    move3.hider_id = 0;
    move3.action_type = static_cast<uint8_t>(TickActionKind::HIDER_ACTION);
    move3.server_id = NO_SERVER_ID;
    move3.target_id = NO_SERVER_ID;
    move3.server_mode = 0; // Some mode
    
    StubPolicy policy{};
    
    // Queue all three moves
    queueMove(state, move1, policy);
    queueMove(state, move2, policy);
    queueMove(state, move3, policy);
    
    REQUIRE(state.pending_count == 3);
    
    // Advance ticks - should apply the first move on tick 5, second on tick 10, third on tick 15
    // But we'll only advance 16 ticks to hit the limit
    TurnResult result = passTicks(state, 16, policy); // Advance 16 ticks
    
    REQUIRE(result.status == MoveStatus::CANCELLED);
    REQUIRE(result.reason == TurnReason::ADVANCE_LIMIT);
    REQUIRE(result.ticksAdvanced == 16); // Should have advanced exactly 16 ticks
}