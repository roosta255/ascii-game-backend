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
    }
};

TEST_CASE("playMove with 0 AP playing PAY_GOAL applies on tick 5", "[turn]") {
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
    
    // Set up server
    state.servers[0].active = true;
    state.servers[0].owner_hider_id = NO_SERVER_ID;
    
    // Create move request for PAY_GOAL (cost 10)
    MoveRequest move{};
    move.move_id = 12345;
    move.hider_id = 0;
    move.action_type = static_cast<uint8_t>(TickActionKind::HIDER_ACTION);
    move.server_id = NO_SERVER_ID;
    move.target_id = NO_SERVER_ID;
    move.server_mode = 0; // PAY_GOAL action
    
    StubPolicy policy{};
    
    // Test playMove - should be affordable within 5 ticks (2 AP/tick regen)
    TurnResult result = playMove(state, move, policy);
    
    REQUIRE(result.status == MoveStatus::APPLIED);
    REQUIRE(result.ticksAdvanced == 5); // Should advance 5 ticks to afford the action
    REQUIRE(policy.invocationCount == 5); // Policy should be called 5 times (once per tick)
}

TEST_CASE("playMove with 0 LOA playing ADD_SERVER returns NEVER_AFFORDABLE", "[turn]") {
    MatchState state{};
    
    // Initialize a basic state
    state.schema_version = 2;
    state.phase = PHASE_ACTIVE;
    state.hider_count = 1;
    state.server_count = 2;
    
    // Set up hider with no LOA (should not be immediately affordable)
    state.hiders[0].active = true;
    state.hiders[0].status = HIDER_STATUS_ACTIVE;
    state.hiders[0].action_points = 0; // No AP
    
    // Set up server
    state.servers[0].active = true;
    state.servers[0].owner_hider_id = NO_SERVER_ID;
    
    // Create move request for ADD_SERVER (costs LOA)
    MoveRequest move{};
    move.move_id = 12345;
    move.hider_id = 0;
    move.action_type = static_cast<uint8_t>(TickActionKind::ADD_SERVER);
    move.server_id = NO_SERVER_ID;
    move.target_id = NO_SERVER_ID;
    move.server_mode = 0; // Some mode
    
    StubPolicy policy{};
    
    // Store original state for comparison
    MatchState original_state = state;
    
    // Test playMove - should return NEVER_AFFORDABLE and not change the state
    TurnResult result = playMove(state, move, policy);
    
    REQUIRE(result.status == MoveStatus::REJECTED);
    REQUIRE(result.reason == TurnReason::NEVER_AFFORDABLE);
    
    // State should be completely unchanged
    REQUIRE(state.schema_version == original_state.schema_version);
    REQUIRE(state.ruleset_version == original_state.ruleset_version);
    REQUIRE(state.revision == original_state.revision);
    REQUIRE(state.tick == original_state.tick);
    REQUIRE(state.rng_state == original_state.rng_state);
    REQUIRE(state.phase == original_state.phase);
    REQUIRE(state.hider_count == original_state.hider_count);
    REQUIRE(state.server_count == original_state.server_count);
    
    // Check hiders unchanged
    for (int i = 0; i < MAX_HIDERS; ++i) {
        REQUIRE(state.hiders[i].active == original_state.hiders[i].active);
        REQUIRE(state.hiders[i].status == original_state.hiders[i].status);
        REQUIRE(state.hiders[i].action_points == original_state.hiders[i].action_points);
    }
    
    // Check servers unchanged
    for (int i = 0; i < MAX_SERVERS; ++i) {
        REQUIRE(state.servers[i].active == original_state.servers[i].active);
        REQUIRE(state.servers[i].owner_hider_id == original_state.servers[i].owner_hider_id);
    }
}