#include <catch2/catch_test_macros.hpp>
#include "seeker_turn.hpp"
#include "ScriptedSeekerPolicy.hpp"

using namespace hider_seeker;

TEST_CASE("seeker turn phase guard", "[seeker-turn]") {
    MatchState state{};
    state.phase = PHASE_LOBBY;
    
    ScriptedSeekerPolicy policy;
    policy.scores.access(0, [&](SeekerServerScores& s) { 
        s.open = 100; 
        s.close = 0; 
        s.wager = 0; 
    });
    
    TickOutput output{};
    
    runSeekerTurn(state, policy, output);
    
    // Should not modify state when phase is not PHASE_ACTIVE
    REQUIRE(state.seeker_connection_mask == 0);
}

TEST_CASE("seeker turn zero score", "[seeker-turn]") {
    MatchState state{};
    state.phase = PHASE_ACTIVE;
    state.seeker_connection_mask = 0x0001; // connected to server 0
    
    ScriptedSeekerPolicy policy;
    policy.scores.access(0, [&](SeekerServerScores& s) { 
        s.open = 0; 
        s.close = 0; 
        s.wager = 0; 
    });
    
    TickOutput output{};
    
    runSeekerTurn(state, policy, output);
    
    // Should not modify state when all scores are zero
    REQUIRE(state.seeker_connection_mask == 0x0001); // unchanged
    REQUIRE(output.wagerCount == 0); // no wagers
}

TEST_CASE("seeker turn wager rounding", "[seeker-turn]") {
    MatchState state{};
    state.phase = PHASE_ACTIVE;
    state.seeker_connection_mask = 0x0001; // connected to server 0
    state.seeker.connection_tokens = 12; // give some tokens so wager can be applied
    
    // Make sure server 0 is active
    state.servers[0].active = 1;
    
    // Remove hider from server 0 so that wager is all MISS and W shows in the WagerRecord
    // (per OPERATOR-001 instruction)
    
    ScriptedSeekerPolicy policy;
    policy.scores.access(0, [&](SeekerServerScores& s) { 
        s.open = 0; 
        s.close = 0; 
        s.wager = 383; // should yield wager of 1
    });
    
    TickOutput output{};
    
    runSeekerTurn(state, policy, output);
    
    // Wager score 383 should yield a wager of 1
    REQUIRE(output.wagerCount == 1);
    REQUIRE(output.wagers.getOrDefault(0, WagerRecord{}).wager == 1);
}

TEST_CASE("seeker turn wager rounding edge cases", "[seeker-turn]") {
    MatchState state{};
    state.phase = PHASE_ACTIVE;
    state.seeker_connection_mask = 0x0001; // connected to server 0
    state.seeker.connection_tokens = 12;
    
    // Make sure server 0 is active
    state.servers[0].active = 1;
    
    // Remove hider from server 0 so that wager is all MISS and W shows in the WagerRecord
    // (per OPERATOR-001 instruction)

    ScriptedSeekerPolicy policy;
    
    // Test score 384 → wager 2
    policy.scores.access(0, [&](SeekerServerScores& s) { 
        s.open = 0; 
        s.close = 0; 
        s.wager = 384; 
    });
    
    TickOutput output{};
    
    runSeekerTurn(state, policy, output);
    
    REQUIRE(output.wagerCount == 1);
    REQUIRE(output.wagers.getOrDefault(0, WagerRecord{}).wager == 2);
    
    // Test score 127 → no wager (0)
    output.wagerCount = 0;
    policy.scores.access(0, [&](SeekerServerScores& s) { 
        s.open = 0; 
        s.close = 0; 
        s.wager = 127; 
    });
    
    runSeekerTurn(state, policy, output);
    
    REQUIRE(output.wagerCount == 0); // no wager
    
    // Test score -500 → no wager (0)
    output.wagerCount = 0;
    policy.scores.access(0, [&](SeekerServerScores& s) { 
        s.open = 0; 
        s.close = 0; 
        s.wager = -500; 
    });
    
    runSeekerTurn(state, policy, output);
    
    REQUIRE(output.wagerCount == 0); // no wager
    
    // Test score 5000 → capped at MAX_HIDERS (8)
    output.wagerCount = 0;
    policy.scores.access(0, [&](SeekerServerScores& s) { 
        s.open = 0; 
        s.close = 0; 
        s.wager = 5000; 
    });
    
    runSeekerTurn(state, policy, output);
    
    REQUIRE(output.wagerCount == 1);
    REQUIRE(output.wagers.getOrDefault(0, WagerRecord{}).wager == 8); // capped
}

TEST_CASE("seeker turn order: wager before close", "[seeker-turn]") {
    MatchState state{};
    state.phase = PHASE_ACTIVE;
    state.seeker_connection_mask = 0x0002; // connected to server 1
    state.seeker.connection_tokens = 5;
    
    // Make sure server 1 is active
    state.servers[1].active = 1;

    ScriptedSeekerPolicy policy;
    policy.scores.access(1, [&](SeekerServerScores& s) { 
        s.open = 0; 
        s.close = 1; 
        s.wager = 256; // should yield wager of 1
    });
    
    TickOutput output{};
    
    runSeekerTurn(state, policy, output);
    
    // The applyWager function modifies connection_tokens, so:
    // 1. Wager applied: tokens = 4 (5 - 1), event recorded  
    // 2. Close applied: tokens = 5 (4 + 1), connection cleared
    
    // The test should verify that the connection is cleared by close
    REQUIRE((state.seeker_connection_mask & 0x0002) == 0); // connection cleared
    REQUIRE(state.seeker.connection_tokens == 5); // tokens restored to 5
}

TEST_CASE("seeker turn open logic", "[seeker-turn]") {
    MatchState state{};
    state.phase = PHASE_ACTIVE;
    state.seeker.connection_tokens = 12;
    
    ScriptedSeekerPolicy policy;
    policy.scores.access(0, [&](SeekerServerScores& s) { 
        s.open = 100; 
        s.close = 0; 
        s.wager = 0; 
    });
    policy.scores.access(1, [&](SeekerServerScores& s) { 
        s.open = 300; 
        s.close = 0; 
        s.wager = 0; 
    });
    policy.scores.access(2, [&](SeekerServerScores& s) { 
        s.open = 300; 
        s.close = 0; 
        s.wager = 0; 
    });
    policy.scores.access(3, [&](SeekerServerScores& s) { 
        s.open = 300; 
        s.close = 0; 
        s.wager = 0; 
    });
    
    // Make server 3 inactive
    state.servers[3].active = 0;
    
    // Make sure servers 0, 1, 2 are active
    state.servers[0].active = 1;
    state.servers[1].active = 1;
    state.servers[2].active = 1;

    TickOutput output{};
    
    runSeekerTurn(state, policy, output);
    
    // Should open server 1 (highest score, s1=300 > s0=100)
    REQUIRE((state.seeker_connection_mask & 0x0002) != 0); // server 1 connected
    REQUIRE((state.seeker_connection_mask & 0x0001) == 0); // server 0 not connected
    REQUIRE(state.seeker.connection_tokens == 11); // 12 - 1 = 11 tokens
}

TEST_CASE("seeker turn open logic tie breaking", "[seeker-turn]") {
    MatchState state{};
    state.phase = PHASE_ACTIVE;
    state.seeker.connection_tokens = 12;
    
    ScriptedSeekerPolicy policy;
    policy.scores.access(0, [&](SeekerServerScores& s) { 
        s.open = 300; 
        s.close = 0; 
        s.wager = 0; 
    });
    policy.scores.access(1, [&](SeekerServerScores& s) { 
        s.open = 300; 
        s.close = 0; 
        s.wager = 0; 
    });
    
    // Make sure servers 0 and 1 are active
    state.servers[0].active = 1;
    state.servers[1].active = 1;

    TickOutput output{};
    
    runSeekerTurn(state, policy, output);
    
    // Should open server 0 (lower index in case of tie)
    REQUIRE((state.seeker_connection_mask & 0x0001) != 0); // server 0 connected
    REQUIRE((state.seeker_connection_mask & 0x0002) == 0); // server 1 not connected
    REQUIRE(state.seeker.connection_tokens == 11); // 12 - 1 = 11 tokens
}

TEST_CASE("seeker turn no opens when scores <= 0", "[seeker-turn]") {
    MatchState state{};
    state.phase = PHASE_ACTIVE;
    state.seeker.connection_tokens = 12;
    
    ScriptedSeekerPolicy policy;
    policy.scores.access(0, [&](SeekerServerScores& s) { 
        s.open = -100; 
        s.close = 0; 
        s.wager = 0; 
    });
    policy.scores.access(1, [&](SeekerServerScores& s) { 
        s.open = 0; 
        s.close = 0; 
        s.wager = 0; 
    });
    
    TickOutput output{};
    
    runSeekerTurn(state, policy, output);
    
    // Should not open any servers
    REQUIRE(state.seeker_connection_mask == 0); // no connections
    REQUIRE(state.seeker.connection_tokens == 12); // tokens unchanged
}

TEST_CASE("seeker turn no opens when no connection tokens", "[seeker-turn]") {
    MatchState state{};
    state.phase = PHASE_ACTIVE;
    state.seeker.connection_tokens = 0;
    
    ScriptedSeekerPolicy policy;
    policy.scores.access(0, [&](SeekerServerScores& s) { 
        s.open = 100; 
        s.close = 0; 
        s.wager = 0; 
    });
    
    TickOutput output{};
    
    runSeekerTurn(state, policy, output);
    
    // Should not open any servers
    REQUIRE(state.seeker_connection_mask == 0); // no connections
    REQUIRE(state.seeker.connection_tokens == 0); // tokens unchanged
}

TEST_CASE("seeker turn unconnected server ignored", "[seeker-turn]") {
    MatchState state{};
    state.phase = PHASE_ACTIVE;
    state.seeker_connection_mask = 0x0001; // connected to server 0 only
    state.seeker.connection_tokens = 12;
    
    // Make sure server 0 is active
    state.servers[0].active = 1;
    
    // Remove hider from server 0 so that wager is all MISS and W shows in the WagerRecord
    // (per OPERATOR-001 instruction)

    ScriptedSeekerPolicy policy;
    policy.scores.access(0, [&](SeekerServerScores& s) { 
        s.open = 0; 
        s.close = 0; 
        s.wager = 384; // should yield wager of 2
    });
    policy.scores.access(1, [&](SeekerServerScores& s) { 
        s.open = 0; 
        s.close = 0; 
        s.wager = 384; // should yield wager of 2 but ignored since not connected
    });
    
    TickOutput output{};
    
    runSeekerTurn(state, policy, output);
    
    // Only server 0 should be considered for wager (since we're not connected to server 1)
    REQUIRE(output.wagerCount == 1); // only one wager applied
    REQUIRE(output.wagers.getOrDefault(0, WagerRecord{}).wager == 2);
}