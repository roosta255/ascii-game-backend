#include <catch2/catch_test_macros.hpp>
#include "tick.hpp"
#include "turn_engine.hpp"
#include "match_end.hpp"
#include "ScriptedSeekerPolicy.hpp"

using namespace hider_seeker;

// Include the enum for MatchWinner
using MatchWinner = hider_seeker::MatchWinner;

namespace {
    MatchState makeActiveMatch() {
        MatchState state{};
        state.tick = 1;
        state.phase = PHASE_ACTIVE;
        state.hider_count = 1;
        state.server_count = 2;
        state.seeker.connection_tokens = 12; // 5a5b: a seeker with no pool and no connection has lost (checkMatchEnd)
        
        state.hiders[0].active = 1;
        state.hiders[0].action_points = 10;
        state.hiders[0].server_id = NO_SERVER_ID; // Not connected initially
        
        state.servers[0].active = 1;
        state.servers[0].current.cpu = 50;
        state.servers[0].current.memory = 50;
        
        state.servers[1].active = 1;
        state.servers[1].current.cpu = 60;
        state.servers[1].current.memory = 60;
        
        return state;
    }
}

TEST_CASE("CONN-006b (match end in runTick): hider destroyed by wager", "[tick]") {
    MatchState state = makeActiveMatch();
    
    // Set up a scripted policy that opens server 0 with open=256 and wager=0
    ScriptedSeekerPolicy policy;
    policy.scores.assignValue(0, SeekerServerScores{256, 0, 0}); // Open server 0
    policy.scores.assignValue(1, SeekerServerScores{0, 0, 0}); // No action on server 1
    
    Array<TickIntent, MAX_INTENTS_PER_TICK> intents;
    uint8_t intentCount = 0;
    ConsumedKeySet alreadyConsumed{};
    
    // First tick: opens server 0 (match stays active)
    TickOutput output = runTick(state, intents, intentCount, alreadyConsumed, &policy);
    REQUIRE(output.wagerCount == 0); // No wagers in first tick
    REQUIRE(state.phase == PHASE_ACTIVE); // Match still active
    
    // Second tick: wagers on server 0 to destroy hider connection
    policy.scores.assignValue(0, SeekerServerScores{0, 256, 0}); // Wager on server 0
    output = runTick(state, intents, intentCount, alreadyConsumed, &policy);
    
    // After second tick, hider should be destroyed and match complete
    REQUIRE(state.phase == PHASE_COMPLETE);
    REQUIRE(static_cast<uint8_t>(state.winner) == static_cast<uint8_t>(MatchWinner::SEEKER));
}

TEST_CASE("CONN-006b (match end in runTick): seeker loses with no tokens", "[tick]") {
    MatchState state = makeActiveMatch();
    state.seeker.connection_tokens = 0; // No tokens
    state.seeker_connection_mask = 0;   // No mask
    
    // Set up a scripted policy that does nothing (all scores 0)
    ScriptedSeekerPolicy policy;
    policy.scores.assignValue(0, SeekerServerScores{0, 0, 0}); // No action
    policy.scores.assignValue(1, SeekerServerScores{0, 0, 0}); // No action
    
    Array<TickIntent, MAX_INTENTS_PER_TICK> intents;
    uint8_t intentCount = 0;
    ConsumedKeySet alreadyConsumed{};
    
    TickOutput output = runTick(state, intents, intentCount, alreadyConsumed, &policy);
    
    // With no tokens and no connection, seeker has lost
    REQUIRE(static_cast<uint8_t>(state.winner) == static_cast<uint8_t>(MatchWinner::HIDERS));
}

TEST_CASE("CONN-006b (match end in runTick): passTicks with scripted policy", "[tick]") {
    MatchState state = makeActiveMatch();
    
    // Set up a scripted policy that opens server 0 with open=256 and wager=0
    ScriptedSeekerPolicy policy;
    policy.scores.assignValue(0, SeekerServerScores{256, 0, 0}); // Open server 0
    policy.scores.assignValue(1, SeekerServerScores{0, 0, 0}); // No action on server 1
    
    // Call passTicks with the same scripted policy
    TurnResult result = passTicks(state, 5, policy);
    
    REQUIRE(result.ticksAdvanced == 2); // Should advance 2 ticks before match ends
    REQUIRE(result.reason == TurnReason::MATCH_NOT_ACTIVE);
    REQUIRE(static_cast<uint8_t>(state.winner) == static_cast<uint8_t>(MatchWinner::SEEKER));
    REQUIRE(result.probesIssued == 1); // One wager issued (from the second tick)
}