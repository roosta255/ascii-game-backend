#include <catch2/catch_test_macros.hpp>
#include "feature_extract.hpp"
#include "state.hpp"
#include "feature_schema.hpp"

using namespace hider_seeker;

TEST_CASE("FeatureExtractTest: connected server with metrics", "[feature_extract]") {
    MatchState state = {};
    
    // Set up server 0 as active and connected
    state.servers[0].active = 1;
    state.seeker_connection_mask = 1; // Connected to server 0
    
    // Set current metrics for server 0
    state.servers[0].current.cpu = 50;
    
    // Set up history frames for server 0 (k=1 and k=2)
    state.servers[0].history_head = 3; // Next write position is 3
    
    // frameAgo(k=1) - slot (3-1-1)%16 = 1
    state.servers[0].history[1].cpu = 40;
    
    // frameAgo(k=2) - slot (3-1-2)%16 = 0  
    state.servers[0].history[0].cpu = 30;
    
    auto obs = buildSeekerObservation(state);
    
    // Test feature index 2 (current cpu) equals 128 (50 * 256 / 100)
    REQUIRE(obs.server.getOrDefault(0, {}).getOrDefault(2, int16_t{0}) == 128);
    
    // Test feature index 9 (cpu Δ1) equals 25 (50 - 40 = 10, 10 * 256 / 100 = 25)
    REQUIRE(obs.server.getOrDefault(0, {}).getOrDefault(9, int16_t{0}) == 25);
    
    // Test feature index 16 (cpu Δ2) equals 51 (50 - 30 = 20, 20 * 256 / 100 = 51)
    REQUIRE(obs.server.getOrDefault(0, {}).getOrDefault(16, int16_t{0}) == 51);
}

TEST_CASE("FeatureExtractTest: inactive server", "[feature_extract]") {
    MatchState state = {};
    
    // Set up server 0 as inactive
    state.servers[0].active = 0;
    
    auto obs = buildSeekerObservation(state);
    
    // All features for inactive server should be 0
    const auto& server_features = obs.server.getOrDefault(0, {});
    for (int i = 0; i < SEEKER_SERVER_FEATURES; ++i) {
        REQUIRE(server_features.getOrDefault(i, int16_t{0}) == 0);
    }
}

TEST_CASE("FeatureExtractTest: active but not connected server", "[feature_extract]") {
    MatchState state = {};
    
    // Set up server 1 as active but not connected
    state.servers[1].active = 1;
    // No connection mask set for server 1
    
    // Set current metrics for server 1
    state.servers[1].current.cpu = 90;
    
    auto obs = buildSeekerObservation(state);
    
    const auto& server_features = obs.server.getOrDefault(1, {});
    
    // Feature 0 (active) should be 256
    REQUIRE(server_features.getOrDefault(0, int16_t{0}) == 256);
    
    // Feature 1 (seeker connected) should be 0
    REQUIRE(server_features.getOrDefault(1, int16_t{0}) == 0);
    
    // Features 2-36 should all be 0 since not connected
    for (int i = 2; i <= 36; ++i) {
        REQUIRE(server_features.getOrDefault(i, int16_t{0}) == 0);
    }
}

TEST_CASE("FeatureExtractTest: global features", "[feature_extract]") {
    MatchState state = {};
    
    // Set up global features
    state.seeker.connection_tokens = 12;
    state.seeker_connection_mask = 1; // One open connection (server 0)
    state.server_count = 2;
    state.tick = 7;
    
    auto obs = buildSeekerObservation(state);
    
    // Global feature 0 (seeker pool) should be min(12, 32) * 8 = 96
    REQUIRE(obs.global.getOrDefault(0, int16_t{0}) == 96);
    
    // Global feature 1 (open seeker connections) should be popcount(1) * 16 = 16
    REQUIRE(obs.global.getOrDefault(1, int16_t{0}) == 16);
    
    // Global feature 2 (active servers) should be 2 * 16 = 32
    REQUIRE(obs.global.getOrDefault(2, int16_t{0}) == 32);
    
    // Global feature 3 (tick) should be min(7, 256) = 7
    REQUIRE(obs.global.getOrDefault(3, int16_t{0}) == 7);
}