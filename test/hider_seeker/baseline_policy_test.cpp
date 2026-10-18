#include <catch2/catch_test_macros.hpp>
#include "baseline_policy.hpp"
#include "feature_schema.hpp"
#include "test/hider_seeker/ScriptedSeekerPolicy.hpp"

using namespace hider_seeker;

TEST_CASE("BaselinePolicyTest: connected server with activity >= threshold", "[baseline_policy]") {
    BaselineSeekerPolicy policy;
    
    // Create a SeekerObservation
    SeekerObservation obs{};
    
    // Set up server 0 as active and connected
    obs.server.access(0, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& row) {
        row.set(0, int16_t{256}); // active = 256
        row.set(1, int16_t{256}); // connected = 256
        
        // Set up features for activity calculation:
        // errors Δ2 = -25 (index 21)
        row.set(21, int16_t{-25});
        
        // network Δ2 = 0 (index 19) 
        row.set(19, int16_t{0});
        
        // heat Δ2 = 0 (index 20)
        row.set(20, int16_t{0});
        
        // cpu Δ2 = 0 (index 16)
        row.set(16, int16_t{0});
        
        // ticks since last wager = 32 (index 37)
        row.set(37, int16_t{32});
    });
    
    // Global features
    obs.global.access(0, [&](int16_t& value) { value = int16_t{10}; }); // seeker pool
    
    Array<SeekerServerScores, MAX_SERVERS> scores;
    
    policy.score(obs, scores);
    
    // Activity = 3*|errors Δ2| + 2*|network Δ2| + 2*|heat Δ2| + 1*|cpu Δ2|
    // Activity = 3*25 + 2*0 + 2*0 + 1*0 = 75 >= 20 (threshold)
    // ticksSinceLastWager = 32 >= 16 (rewager gap)
    // Should set wager to 256
    const auto& result = scores.getOrDefault(0, SeekerServerScores{});
    
    // Debug: Let's check what the policy actually sees
    bool isActive = false;
    obs.server.accessConst(0, [&](const Array<int16_t, SEEKER_SERVER_FEATURES>& row) {
        int16_t activeValue = row.getOrDefault(0, int16_t{0});
        isActive = (activeValue == 256);
    });
    
    // This should be true
    REQUIRE(isActive == true);
    REQUIRE(result.wager == 256);
}

TEST_CASE("BaselinePolicyTest: connected server with activity < threshold", "[baseline_policy]") {
    BaselineSeekerPolicy policy;
    
    // Create a SeekerObservation
    SeekerObservation obs{};
    
    // Set up server 0 as active and connected
    obs.server.access(0, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& row) {
        row.set(0, int16_t{256}); // active = 256
        row.set(1, int16_t{256}); // connected = 256
        
        // Set up features for activity calculation:
        // errors Δ2 = -25 (index 21)
        row.set(21, int16_t{-25});
        
        // network Δ2 = 0 (index 19) 
        row.set(19, int16_t{0});
        
        // heat Δ2 = 0 (index 20)
        row.set(20, int16_t{0});
        
        // cpu Δ2 = 0 (index 16)
        row.set(16, int16_t{0});
        
        // ticks since last wager = 8 (index 37) - less than threshold
        row.set(37, int16_t{8});
    });
    
    // Global features
    obs.global.access(0, [&](int16_t& value) { value = int16_t{10}; }); // seeker pool
    
    Array<SeekerServerScores, MAX_SERVERS> scores;
    
    policy.score(obs, scores);
    
    // Activity = 3*25 + 2*0 + 2*0 + 1*0 = 75 >= 20 (threshold)
    // But ticksSinceLastWager = 8 < 16 (rewager gap)
    // Should set wager to 0
    const auto& result = scores.getOrDefault(0, SeekerServerScores{});
    REQUIRE(result.wager == 0);
}

TEST_CASE("BaselinePolicyTest: connected server with activity < threshold 2", "[baseline_policy]") {
    BaselineSeekerPolicy policy;
    
    // Create a SeekerObservation
    SeekerObservation obs{};
    
    // Set up server 0 as active and connected
    obs.server.access(0, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& row) {
        row.set(0, int16_t{256}); // active = 256
        row.set(1, int16_t{256}); // connected = 256
        
        // Set up features for activity calculation:
        // errors Δ2 = -1 (index 21) - this makes activity = 3*1 + 2*0 + 2*0 + 1*0 = 3 < 20
        row.set(21, int16_t{-1});
        
        // network Δ2 = 0 (index 19) 
        row.set(19, int16_t{0});
        
        // heat Δ2 = 0 (index 20)
        row.set(20, int16_t{0});
        
        // cpu Δ2 = 0 (index 16)
        row.set(16, int16_t{0});
        
        // ticks since last wager = 32 (index 37)
        row.set(37, int16_t{32});
    });
    
    // Global features
    obs.global.access(0, [&](int16_t& value) { value = int16_t{10}; }); // seeker pool
    
    Array<SeekerServerScores, MAX_SERVERS> scores;
    
    policy.score(obs, scores);
    
    // Activity = 3*1 + 2*0 + 2*0 + 1*0 = 3 < 20 (threshold)
    // Should set wager to 0
    const auto& result = scores.getOrDefault(0, SeekerServerScores{});
    REQUIRE(result.wager == 0);
}

TEST_CASE("BaselinePolicyTest: not connected server with open conditions met", "[baseline_policy]") {
    BaselineSeekerPolicy policy;
    
    // Create a SeekerObservation
    SeekerObservation obs{};
    
    // Set up server 0 as active but not connected
    obs.server.access(0, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& row) {
        row.set(0, int16_t{256}); // active = 256
        row.set(1, int16_t{0});   // connected = 0
        
        // ticks since last wager = 24 (index 37)
        row.set(37, int16_t{24});
    });
    
    // Global features
    obs.global.access(1, [&](int16_t& value) { value = int16_t{16}; }); // open connections < 2*16 = 32
    
    Array<SeekerServerScores, MAX_SERVERS> scores;
    
    policy.score(obs, scores);
    
    // Should set open to 256 + 24 = 280
    const auto& result = scores.getOrDefault(0, SeekerServerScores{});
    REQUIRE(result.open == 280);
}

TEST_CASE("BaselinePolicyTest: not connected server with open conditions not met", "[baseline_policy]") {
    BaselineSeekerPolicy policy;
    
    // Create a SeekerObservation
    SeekerObservation obs{};
    
    // Set up server 0 as active but not connected
    obs.server.access(0, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& row) {
        row.set(0, int16_t{256}); // active = 256
        row.set(1, int16_t{0});   // connected = 0
        
        // ticks since last wager = 24 (index 37)
        row.set(37, int16_t{24});
    });
    
    // Global features
    obs.global.access(1, [&](int16_t& value) { value = int16_t{32}; }); // open connections >= 2*16 = 32
    
    Array<SeekerServerScores, MAX_SERVERS> scores;
    
    policy.score(obs, scores);
    
    // Should set open to 0 (conditions not met)
    const auto& result = scores.getOrDefault(0, SeekerServerScores{});
    REQUIRE(result.open == 0);
}

TEST_CASE("BaselinePolicyTest: connected server with empty seeker pool", "[baseline_policy]") {
    BaselineSeekerPolicy policy;
    
    // Create a SeekerObservation
    SeekerObservation obs{};
    
    // Set up server 0 as active and connected
    obs.server.access(0, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& row) {
        row.set(0, int16_t{256}); // active = 256
        row.set(1, int16_t{256}); // connected = 256
    });
    
    // Global features - seeker pool is empty
    obs.global.access(0, [&](int16_t& value) { value = int16_t{0}; }); // seeker pool = 0
    
    Array<SeekerServerScores, MAX_SERVERS> scores;
    
    policy.score(obs, scores);
    
    // Should set close to 256 (pool is empty)
    const auto& result = scores.getOrDefault(0, SeekerServerScores{});
    REQUIRE(result.close == 256);
}

TEST_CASE("BaselinePolicyTest: inactive server", "[baseline_policy]") {
    BaselineSeekerPolicy policy;
    
    // Create a SeekerObservation
    SeekerObservation obs{};
    
    // Set up server 0 as inactive
    obs.server.access(0, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& row) {
        row.set(0, int16_t{0}); // active = 0
    });
    
    Array<SeekerServerScores, MAX_SERVERS> scores;
    
    policy.score(obs, scores);
    
    // Should set all scores to 0 (inactive server)
    const auto& result = scores.getOrDefault(0, SeekerServerScores{});
    REQUIRE(result.open == 0);
    REQUIRE(result.close == 0);
    REQUIRE(result.wager == 0);
}

TEST_CASE("BaselinePolicyTest: policyId returns baseline", "[baseline_policy]") {
    BaselineSeekerPolicy policy;
    
    REQUIRE(policy.policyId() == "baseline");
}