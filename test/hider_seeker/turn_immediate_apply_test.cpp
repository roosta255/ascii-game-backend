#include <catch2/catch_test_macros.hpp>
#include "../../src/hider_seeker/domain/state.hpp"
#include "../../src/hider_seeker/turn/turn_engine.hpp"
#include "../../src/hider_seeker/tick/tick.hpp"

using namespace hider_seeker;

// Stub policy that counts invocations
struct StubPolicy : public iTargetRankingPolicy {
    int invocationCount = 0;
    
    void rankTargets(const MatchState& state, Array<ProbeCandidate, MAX_SERVERS>& candidates) override {
        invocationCount++;
        // Do nothing - just count the call
    }
};

TEST_CASE("playMove/queueMove immediate apply makes zero policy calls", "[turn]") {
    MatchState state{};
    
    // Initialize a basic state
    state.schema_version = 2;
    state.phase = PHASE_ACTIVE;
    state.hider_count = 1;
    state.server_count = 2;
    
    // Set up hider with enough AP to make an immediate action
    state.hiders[0].active = true;
    state.hiders[0].status = HIDER_STATUS_ACTIVE;
    state.hiders[0].action_points = 5; // Enough for a basic action
    
    // Set up server
    state.servers[0].active = true;
    state.servers[0].owner_hider_id = NO_SERVER_ID;
    
    // Create move request that should be immediately affordable
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
    
    REQUIRE(result1.status == MoveStatus::APPLIED);
    REQUIRE(result1.ticksAdvanced == 0);
    REQUIRE(result1.reason == TurnReason::NONE);
    REQUIRE(policy.invocationCount == 0); // Should not call policy
    
    // Test queueMove (should also be immediately affordable)
    StubPolicy policy2{};
    TurnResult result2 = queueMove(state, move, policy2);
    
    REQUIRE(result2.status == MoveStatus::QUEUED);
    REQUIRE(result2.reason == TurnReason::NONE);
    REQUIRE(policy2.invocationCount == 0); // Should not call policy
}

TEST_CASE("playMove/queueMove with tick unchanged", "[turn]") {
    MatchState state{};
    
    // Initialize a basic state
    state.schema_version = 2;
    state.phase = PHASE_ACTIVE;
    state.tick = 100;
    state.hider_count = 1;
    state.server_count = 2;
    
    // Set up hider with enough AP to make an immediate action
    state.hiders[0].active = true;
    state.hiders[0].status = HIDER_STATUS_ACTIVE;
    state.hiders[0].action_points = 5; // Enough for a basic action
    
    // Set up server
    state.servers[0].active = true;
    state.servers[0].owner_hider_id = NO_SERVER_ID;
    
    // Create move request that should be immediately affordable
    MoveRequest move{};
    move.move_id = 12345;
    move.hider_id = 0;
    move.action_type = static_cast<uint8_t>(TickActionKind::HIDER_ACTION);
    move.server_id = NO_SERVER_ID;
    move.target_id = NO_SERVER_ID;
    move.server_mode = 0; // Some mode
    
    StubPolicy policy{};
    
    uint32_t original_tick = state.tick;
    
    // Test playMove
    TurnResult result1 = playMove(state, move, policy);
    
    REQUIRE(result1.status == MoveStatus::APPLIED);
    REQUIRE(result1.ticksAdvanced == 0);
    REQUIRE(state.tick == original_tick); // Tick should not change
    
    // Test queueMove
    uint32_t original_tick2 = state.tick;
    TurnResult result2 = queueMove(state, move, policy);
    
    REQUIRE(result2.status == MoveStatus::QUEUED);
    REQUIRE(state.tick == original_tick2); // Tick should not change
}