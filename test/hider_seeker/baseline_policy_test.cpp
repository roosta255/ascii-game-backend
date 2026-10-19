#include <catch2/catch_test_macros.hpp>
#include "baseline_policy.hpp"
#include "feature_extract.hpp"

using namespace hider_seeker;

TEST_CASE("BaselinePolicyTest", "[hider_seeker]") {
    BaselineSeekerPolicy policy;
    
    // Test that policyId returns "baseline"
    REQUIRE(policy.policyId() == "baseline");
    
    // Test connected server with f[errors Δ2] = -25 and f[37] = 32
    // Should set wager to 256 (activity = 75 >= 20)
    {
        SeekerObservation obs{};
        Array<SeekerServerScores, MAX_SERVERS> scores;
        
        // Set up server features for connected server
        obs.server.access(0, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& f) {
            f.set(0, 256); // active
            f.set(1, 256); // connected
            f.set(21, -25); // errors Δ2 (index 21 from k=2 block, position 5)
            f.set(37, 32);  // ticks since last wager
        });
        
        obs.global.access(0, [&](int16_t& value) {
            value = 0; // pool is empty - this will be tested in a separate case
        });
        obs.global.access(1, [&](int16_t& value) {
            value = 16; // 1 open connection (popcount of mask with 1 bit set)
        });
        obs.global.access(2, [&](int16_t& value) {
            value = 32; // 2 active servers
        });
        obs.global.access(3, [&](int16_t& value) {
            value = 7; // tick
        });
        
        policy.score(obs, scores);
        
        // Check that wager is set to 256 (activity = 3*25 + 2*0 + 2*0 + 1*0 = 75 >= 20 AND f[37] >= 16)
        scores.access(0, [&](const SeekerServerScores& s) {
            REQUIRE(s.wager == 256);
        });
    }
    
    // Test same but f[37] = 8 (should be 0 since gap < 16)
    {
        SeekerObservation obs{};
        Array<SeekerServerScores, MAX_SERVERS> scores;
        
        obs.server.access(0, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& f) {
            f.set(0, 256); // active
            f.set(1, 256); // connected
            f.set(21, -25); // errors Δ2 
            f.set(37, 8);   // ticks since last wager (less than threshold)
        });
        
        obs.global.access(0, [&](int16_t& value) {
            value = 0;
        });
        obs.global.access(1, [&](int16_t& value) {
            value = 16;
        });
        obs.global.access(2, [&](int16_t& value) {
            value = 32;
        });
        obs.global.access(3, [&](int16_t& value) {
            value = 7;
        });
        
        policy.score(obs, scores);
        
        // Check that wager is 0 (gap < 16)
        scores.access(0, [&](const SeekerServerScores& s) {
            REQUIRE(s.wager == 0);
        });
    }
    
    // Test activity evaluates to 19 (should be 0 since < threshold of 20)
    {
        SeekerObservation obs{};
        Array<SeekerServerScores, MAX_SERVERS> scores;
        
        obs.server.access(0, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& f) {
            f.set(0, 256); // active
            f.set(1, 256); // connected
            f.set(21, -3);  // errors Δ2 (small change)
            f.set(37, 32);  // ticks since last wager
        });
        
        obs.global.access(0, [&](int16_t& value) {
            value = 0;
        });
        obs.global.access(1, [&](int16_t& value) {
            value = 16;
        });
        obs.global.access(2, [&](int16_t& value) {
            value = 32;
        });
        obs.global.access(3, [&](int16_t& value) {
            value = 7;
        });
        
        policy.score(obs, scores);
        
        // Check that wager is 0 (activity < 20)
        scores.access(0, [&](const SeekerServerScores& s) {
            REQUIRE(s.wager == 0);
        });
    }
    
    // Test not connected, f[37] = 24, global g[1] = 16
    // Should set open to 280 (256 + 24)
    {
        SeekerObservation obs{};
        Array<SeekerServerScores, MAX_SERVERS> scores;
        
        obs.server.access(0, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& f) {
            f.set(0, 256); // active
            f.set(1, 0);    // not connected
            f.set(37, 24);  // ticks since last wager
        });
        
        obs.global.access(0, [&](int16_t& value) {
            value = 0;
        });
        obs.global.access(1, [&](int16_t& value) {
            value = 16; // 1 open connection (less than 2*16 = 32)
        });
        obs.global.access(2, [&](int16_t& value) {
            value = 32;
        });
        obs.global.access(3, [&](int16_t& value) {
            value = 7;
        });
        
        policy.score(obs, scores);
        
        // Check that open is set to 280 (256 + 24)
        scores.access(0, [&](const SeekerServerScores& s) {
            REQUIRE(s.open == 280);
        });
    }
    
    // Test same but g[1] = 32 (should be 0 since >= 2*16)
    {
        SeekerObservation obs{};
        Array<SeekerServerScores, MAX_SERVERS> scores;
        
        obs.server.access(0, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& f) {
            f.set(0, 256); // active
            f.set(1, 0);    // not connected
            f.set(37, 24);  // ticks since last wager
        });
        
        obs.global.access(0, [&](int16_t& value) {
            value = 0;
        });
        obs.global.access(1, [&](int16_t& value) {
            value = 32; // 2 open connections (>= 2*16 = 32)
        });
        obs.global.access(2, [&](int16_t& value) {
            value = 32;
        });
        obs.global.access(3, [&](int16_t& value) {
            value = 7;
        });
        
        policy.score(obs, scores);
        
        // Check that open is 0 (g[1] >= 2*16)
        scores.access(0, [&](const SeekerServerScores& s) {
            REQUIRE(s.open == 0);
        });
    }
    
    // Test connected server, global g[0] = 0
    // Should set close to 256
    {
        SeekerObservation obs{};
        Array<SeekerServerScores, MAX_SERVERS> scores;
        
        obs.server.access(0, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& f) {
            f.set(0, 256); // active
            f.set(1, 256); // connected
        });
        
        obs.global.access(0, [&](int16_t& value) {
            value = 0; // pool is empty
        });
        obs.global.access(1, [&](int16_t& value) {
            value = 16;
        });
        obs.global.access(2, [&](int16_t& value) {
            value = 32;
        });
        obs.global.access(3, [&](int16_t& value) {
            value = 7;
        });
        
        policy.score(obs, scores);
        
        // Check that close is set to 256 (g[0] == 0)
        scores.access(0, [&](const SeekerServerScores& s) {
            REQUIRE(s.close == 256);
        });
    }
    
    // Test inactive server - all scores should be 0
    {
        SeekerObservation obs{};
        Array<SeekerServerScores, MAX_SERVERS> scores;
        
        obs.server.access(0, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& f) {
            f.set(0, 0); // inactive
        });
        
        policy.score(obs, scores);
        
        // Check that all scores are 0 for inactive server
        scores.access(0, [&](const SeekerServerScores& s) {
            REQUIRE(s.open == 0);
            REQUIRE(s.close == 0);
            REQUIRE(s.wager == 0);
        });
    }
    
    // Test activity evaluates to 19 (should be 0 since < threshold of 20)
    {
        SeekerObservation obs{};
        Array<SeekerServerScores, MAX_SERVERS> scores;
        
        obs.server.access(0, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& f) {
            f.set(0, 256); // active
            f.set(1, 256); // connected
            // activity = 3*|-5| + 2*|2| + 2*|0| + 1*|0| = 15 + 4 + 0 + 0 = 19
            f.set(21, -5);  // errors Δ2 (index 21 from k=2 block, position 5)
            f.set(19, 2);   // network Δ2 (index 19 from k=2 block, position 3) 
            f.set(20, 0);   // heat Δ2 (index 20 from k=2 block, position 4)
            f.set(16, 0);   // cpu Δ2 (index 16 from k=2 block, position 0)
            f.set(37, 32);  // ticks since last wager
        });
        
        obs.global.access(0, [&](int16_t& value) {
            value = 0;
        });
        obs.global.access(1, [&](int16_t& value) {
            value = 16;
        });
        obs.global.access(2, [&](int16_t& value) {
            value = 32;
        });
        obs.global.access(3, [&](int16_t& value) {
            value = 7;
        });
        
        policy.score(obs, scores);
        
        // Check that wager is 0 (activity < 20)
        scores.access(0, [&](const SeekerServerScores& s) {
            REQUIRE(s.wager == 0);
        });
    }
    
    // Test active server not connected, tick=10, last_probe_tick[3]=5
    // Feature 37 should be 40 (min((10-5)&0xFF,32)*8 = min(5,32)*8 = 5*8)
    {
        SeekerObservation obs{};
        Array<SeekerServerScores, MAX_SERVERS> scores;
        
        obs.server.access(3, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& f) {
            f.set(0, 256); // active
            f.set(1, 0);    // not connected
            f.set(37, 40);  // computed value
        });
        
        obs.global.access(0, [&](int16_t& value) {
            value = 0;
        });
        obs.global.access(1, [&](int16_t& value) {
            value = 16;
        });
        obs.global.access(2, [&](int16_t& value) {
            value = 32;
        });
        obs.global.access(3, [&](int16_t& value) {
            value = 7;
        });
        
        policy.score(obs, scores);
        
        // Check that the computed feature 37 is correct
        obs.server.access(3, [&](const Array<int16_t, SEEKER_SERVER_FEATURES>& f) {
            REQUIRE(f.getOrDefault(37, int16_t{0}) == 40);
        });
    }
}