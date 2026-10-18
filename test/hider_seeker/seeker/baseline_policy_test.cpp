#include <catch2/catch_test_macros.hpp>
#include "baseline_policy.hpp"
#include "feature_schema.hpp"
#include "feature_extract.hpp"

using namespace hider_seeker;

TEST_CASE("BASE-001: Baseline policy scoring logic", "[hider_seeker][seeker][policy]") {
    BaselineSeekerPolicy policy;
    
    // Test that policyId returns "baseline"
    REQUIRE(policy.policyId() == "baseline");
    
    // Create a SeekerObservation with test data
    SeekerObservation obs = {};
    
    // Set up server 0 as connected
    obs.server.access(0, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& row) {
        row.set(0, 256); // active
        row.set(1, 256); // seeker connected
        
        // Set up change features for activity calculation
        // errors Δ2 = -25 (index 21)
        row.set(21, -25);
        // network Δ2 = 0 (index 22) 
        row.set(22, 0);
        // heat Δ2 = 0 (index 23)
        row.set(23, 0);
        // cpu Δ2 = 0 (index 24)
        row.set(24, 0);
        
        // ticks since last wager = 32
        row.set(37, 32);
    });
    
    // Set up global features
    obs.global.access([&](Array<int16_t, SEEKER_GLOBAL_FEATURES>& globalRow) {
        globalRow.set(0, 8); // pool = 8 (connection_tokens = 1)
        globalRow.set(1, 16); // open connections = 1 (popcount of mask = 1)
        globalRow.set(2, 32); // active servers = 2
        globalRow.set(3, 7); // tick = 7
    });
    
    Array<SeekerServerScores, MAX_SERVERS> scores;
    
    policy.score(obs, scores);
    
    // Test: connected server with errors Δ2 = -25 and f[37] = 32 should set wager to 256
    // activity = 3*abs(-25) + 2*abs(0) + 2*abs(0) + 1*abs(0) = 75 >= 20 and f[37] = 32 >= 16
    REQUIRE(scores.getOrDefault(0, SeekerServerScores{}).wager == 256);
    
    // Test: same observation but f[37] = 8 should set wager to 0
    obs.server.access(0, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& row) {
        row.set(37, 8); // ticks since last wager = 8 (less than 16)
    });
    
    policy.score(obs, scores);
    REQUIRE(scores.getOrDefault(0, SeekerServerScores{}).wager == 0);
    
    // Test: activity evaluates to 19 should set wager to 0
    obs.server.access(0, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& row) {
        row.set(21, -5); // errors Δ2 = -5 (activity = 3*5 + 2*0 + 2*0 + 1*0 = 15)
        row.set(37, 32); // ticks since last wager = 32
    });
    
    policy.score(obs, scores);
    REQUIRE(scores.getOrDefault(0, SeekerServerScores{}).wager == 0);
    
    // Test: not connected server with f[37] = 24 and g[1] = 16 should set open to 280
    obs.server.access(0, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& row) {
        row.set(1, 0); // not connected
        row.set(37, 24); // ticks since last wager = 24
    });
    
    obs.global.access([&](Array<int16_t, SEEKER_GLOBAL_FEATURES>& globalRow) {
        globalRow.set(1, 16); // open connections = 1 (less than 2*16 = 32)
    });
    
    policy.score(obs, scores);
    REQUIRE(scores.getOrDefault(0, SeekerServerScores{}).open == 280); // 256 + 24
    
    // Test: same but g[1] = 32 should set open to 0
    obs.global.access([&](Array<int16_t, SEEKER_GLOBAL_FEATURES>& globalRow) {
        globalRow.set(1, 32); // open connections = 2 (equal to 2*16 = 32)
    });
    
    policy.score(obs, scores);
    REQUIRE(scores.getOrDefault(0, SeekerServerScores{}).open == 0);
    
    // Test: connected server with g[0] = 0 should set close to 256
    obs.server.access(0, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& row) {
        row.set(1, 256); // connected
    });
    
    obs.global.access([&](Array<int16_t, SEEKER_GLOBAL_FEATURES>& globalRow) {
        globalRow.set(0, 0); // pool = 0 (empty)
    });
    
    policy.score(obs, scores);
    REQUIRE(scores.getOrDefault(0, SeekerServerScores{}).close == 256);
    
    // Test: inactive server should have all scores 0
    obs.server.access(0, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& row) {
        row.set(0, 0); // inactive
    });
    
    policy.score(obs, scores);
    REQUIRE(scores.getOrDefault(0, SeekerServerScores{}).open == 0);
    REQUIRE(scores.getOrDefault(0, SeekerServerScores{}).close == 0);
    REQUIRE(scores.getOrDefault(0, SeekerServerScores{}).wager == 0);
}