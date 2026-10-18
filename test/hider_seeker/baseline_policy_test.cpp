#include <catch2/catch_test_macros.hpp>
#include "baseline_policy.hpp"
#include "ScriptedSeekerPolicy.hpp"

using namespace hider_seeker;

TEST_CASE("BaselinePolicyTest - Connected server with high activity", "[baseline-policy]") {
    BaselineSeekerPolicy policy;
    
    // Create a SeekerObservation manually
    SeekerObservation obs = {};
    
    // Set up server 0 as active and connected
    obs.server.access(0, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& f) {
        f.set(0, int16_t{256});  // Active
        f.set(1, int16_t{256});  // Connected
        f.set(21, int16_t{-25});  // errors Δ2
        f.set(37, int16_t{32});   // ticks since last wager
    });
    
    // Set up global features
    obs.global.access(0, [&](int16_t& value) { value = 10; });      // connection tokens (not zero)
    obs.global.access(1, [&](int16_t& value) { value = 16; });      // open connections (less than 2*16 = 32)
    
    Array<SeekerServerScores, MAX_SERVERS> scores;
    
    policy.score(obs, scores);
    
    // Should wager because activity >= threshold and ticks since last wager >= gap
    REQUIRE(scores.getOrDefault(0, SeekerServerScores{}).wager == 256);
}

TEST_CASE("BaselinePolicyTest - Connected server with low activity", "[baseline-policy]") {
    BaselineSeekerPolicy policy;
    
    // Create a SeekerObservation manually
    SeekerObservation obs = {};
    
    // Set up server 0 as active and connected
    obs.server.access(0, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& f) {
        f.set(0, int16_t{256});  // Active
        f.set(1, int16_t{256});  // Connected
        f.set(21, int16_t{-1});   // errors Δ2
        f.set(37, int16_t{32});   // ticks since last wager
    });
    
    // Set up global features
    obs.global.access(0, [&](int16_t& value) { value = 10; });      // connection tokens (not zero)
    obs.global.access(1, [&](int16_t& value) { value = 16; });      // open connections (less than 2*16 = 32)
    
    Array<SeekerServerScores, MAX_SERVERS> scores;
    
    policy.score(obs, scores);
    
    // Should not wager because activity < threshold
    REQUIRE(scores.getOrDefault(0, SeekerServerScores{}).wager == 0);
}

TEST_CASE("BaselinePolicyTest - Connected server with low ticks since last wager", "[baseline-policy]") {
    BaselineSeekerPolicy policy;
    
    // Create a SeekerObservation manually
    SeekerObservation obs = {};
    
    // Set up server 0 as active and connected
    obs.server.access(0, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& f) {
        f.set(0, int16_t{256});  // Active
        f.set(1, int16_t{256});  // Connected
        f.set(21, int16_t{-25});  // errors Δ2
        f.set(37, int16_t{8});   // ticks since last wager (less than gap of 16)
    });
    
    // Set up global features
    obs.global.access(0, [&](int16_t& value) { value = 10; });      // connection tokens (not zero)
    obs.global.access(1, [&](int16_t& value) { value = 16; });      // open connections (less than 2*16 = 32)
    
    Array<SeekerServerScores, MAX_SERVERS> scores;
    
    policy.score(obs, scores);
    
    // Should not wager because ticks since last wager < gap
    REQUIRE(scores.getOrDefault(0, SeekerServerScores{}).wager == 0);
}

TEST_CASE("BaselinePolicyTest - Not connected server with sufficient open connections", "[baseline-policy]") {
    BaselineSeekerPolicy policy;
    
    // Create a SeekerObservation manually
    SeekerObservation obs = {};
    
    // Set up server 0 as active but not connected
    obs.server.access(0, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& f) {
        f.set(0, int16_t{256});  // Active
        f.set(1, int16_t{0});    // Not connected
        f.set(37, int16_t{24});  // 24 ticks since last wager
    });
    
    // Set up global features with few open connections
    obs.global.access(0, [&](int16_t& value) { value = 10; });      // connection tokens (not zero)
    obs.global.access(1, [&](int16_t& value) { value = 16; });      // open connections (less than 2*16 = 32)
    
    Array<SeekerServerScores, MAX_SERVERS> scores;
    
    policy.score(obs, scores);
    
    // Should open because global[1] < 2*16 and ticks since last wager is 24
    REQUIRE(scores.getOrDefault(0, SeekerServerScores{}).open == 280);  // 256 + 24 = 280
}

TEST_CASE("BaselinePolicyTest - Not connected server with too many open connections", "[baseline-policy]") {
    BaselineSeekerPolicy policy;
    
    // Create a SeekerObservation manually
    SeekerObservation obs = {};
    
    // Set up server 0 as active but not connected
    obs.server.access(0, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& f) {
        f.set(0, int16_t{256});  // Active
        f.set(1, int16_t{0});    // Not connected
        f.set(37, int16_t{24});  // 24 ticks since last wager
    });
    
    // Set up global features with many open connections
    obs.global.access(0, [&](int16_t& value) { value = 10; });      // connection tokens (not zero)
    obs.global.access(1, [&](int16_t& value) { value = 32; });      // open connections (equal to 2*16 = 32)
    
    Array<SeekerServerScores, MAX_SERVERS> scores;
    
    policy.score(obs, scores);
    
    // Should not open because global[1] >= 2*16
    REQUIRE(scores.getOrDefault(0, SeekerServerScores{}).open == 0);
}

TEST_CASE("BaselinePolicyTest - Connected server with empty pool", "[baseline-policy]") {
    BaselineSeekerPolicy policy;
    
    // Create a SeekerObservation manually
    SeekerObservation obs = {};
    
    // Set up server 0 as active and connected
    obs.server.access(0, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& f) {
        f.set(0, int16_t{256});  // Active
        f.set(1, int16_t{256});  // Connected
    });
    
    // Set up global features with empty pool
    obs.global.access(0, [&](int16_t& value) { value = 0; });       // connection tokens (zero)
    obs.global.access(1, [&](int16_t& value) { value = 16; });      // open connections (less than 2*16 = 32)
    
    Array<SeekerServerScores, MAX_SERVERS> scores;
    
    policy.score(obs, scores);
    
    // Should close because pool is empty
    REQUIRE(scores.getOrDefault(0, SeekerServerScores{}).close == 256);
}

TEST_CASE("BaselinePolicyTest - Inactive server", "[baseline-policy]") {
    BaselineSeekerPolicy policy;
    
    // Create a SeekerObservation manually
    SeekerObservation obs = {};
    
    // Set up server 0 as inactive
    obs.server.access(0, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& f) {
        f.set(0, int16_t{0});    // Inactive
    });
    
    Array<SeekerServerScores, MAX_SERVERS> scores;
    
    policy.score(obs, scores);
    
    // Should have all zero scores for inactive server
    const SeekerServerScores& s = scores.getOrDefault(0, SeekerServerScores{});
    REQUIRE(s.open == 0);
    REQUIRE(s.close == 0);
    REQUIRE(s.wager == 0);
}

TEST_CASE("BaselinePolicyTest - Policy ID", "[baseline-policy]") {
    BaselineSeekerPolicy policy;
    
    // Should return the literal string "baseline"
    REQUIRE(policy.policyId() == "baseline");
}