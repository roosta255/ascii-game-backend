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

TEST_CASE("passTicks advances exactly 3 ticks", "[turn]") {
    MatchState state{};
    
    // Initialize a basic state
    state.schema_version = 2;
    state.phase = PHASE_ACTIVE;
    state.tick = 100;
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
    
    StubPolicy policy{};
    
    uint32_t original_tick = state.tick;
    
    // Pass 3 ticks
    TurnResult result = passTicks(state, 3, policy);
    
    REQUIRE(result.status == MoveStatus::APPLIED); // Should be applied (no moves to process)
    REQUIRE(result.ticksAdvanced == 3);
    REQUIRE(state.tick == original_tick + 3); // Tick should advance by exactly 3
}

TEST_CASE("passTicks(0) returns INVALID_TICK_COUNT", "[turn]") {
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
    
    StubPolicy policy{};
    
    // Pass 0 ticks - should be rejected
    TurnResult result = passTicks(state, 0, policy);
    
    REQUIRE(result.status == MoveStatus::REJECTED);
    REQUIRE(result.reason == TurnReason::INVALID_TICK_COUNT);
}

TEST_CASE("passTicks(17) returns INVALID_TICK_COUNT", "[turn]") {
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
    
    StubPolicy policy{};
    
    // Pass 17 ticks - should be rejected (exceeds MAX_ADVANCE_TICKS)
    TurnResult result = passTicks(state, 17, policy);
    
    REQUIRE(result.status == MoveStatus::REJECTED);
    REQUIRE(result.reason == TurnReason::INVALID_TICK_COUNT);
}

TEST_CASE("passTicks determinism with identical call sequences", "[turn]") {
    MatchState state1{};
    MatchState state2{};
    
    // Initialize both states identically
    state1.schema_version = 2;
    state1.phase = PHASE_ACTIVE;
    state1.tick = 100;
    state1.hider_count = 1;
    state1.server_count = 2;
    
    state2.schema_version = 2;
    state2.phase = PHASE_ACTIVE;
    state2.tick = 100;
    state2.hider_count = 1;
    state2.server_count = 2;
    
    // Set up hiders identically
    state1.hiders[0].active = true;
    state1.hiders[0].status = HIDER_STATUS_ACTIVE;
    state1.hiders[0].action_points = 0; // No AP
    
    state2.hiders[0].active = true;
    state2.hiders[0].status = HIDER_STATUS_ACTIVE;
    state2.hiders[0].action_points = 0; // No AP
    
    // Set up servers identically
    state1.servers[0].active = true;
    state1.servers[0].owner_hider_id = NO_SERVER_ID;
    
    state1.servers[1].active = true;
    state1.servers[1].owner_hider_id = NO_SERVER_ID;
    
    state2.servers[0].active = true;
    state2.servers[0].owner_hider_id = NO_SERVER_ID;
    
    state2.servers[1].active = true;
    state2.servers[1].owner_hider_id = NO_SERVER_ID;
    
    StubPolicy policy{};
    
    // Call passTicks(3) on both states
    TurnResult result1 = passTicks(state1, 3, policy);
    TurnResult result2 = passTicks(state2, 3, policy);
    
    // Both should have the same results
    REQUIRE(result1.ticksAdvanced == result2.ticksAdvanced);
    REQUIRE(state1.tick == state2.tick);
}