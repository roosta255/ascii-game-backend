#include <catch2/catch_test_macros.hpp>
#include "state.hpp"
#include "feature_extract.hpp"

using namespace hider_seeker;

TEST_CASE("FeatureExtractTest - Connected server with specific metrics", "[feature-extract]") {
    MatchState state = {};
    
    // Set up a connected server (server 0)
    state.servers[0].active = 1;
    state.seeker_connection_mask = 0b1;  // Connected to server 0
    state.tick = 100;
    state.seeker.last_probe_tick[0] = 80;  // Last probe was 20 ticks ago
    state.seeker.belief[0] = 3;  // Belief is 3
    
    // Set current metrics
    state.servers[0].current.cpu = 50;
    
    // Set up history for frameAgo(k=1) and k=2
    state.servers[0].history_head = 3;
    state.servers[0].history[0].cpu = 40;  // k=2 (frame 0)
    state.servers[0].history[1].cpu = 30;  // k=1 (frame 1) 
    state.servers[0].history[2].cpu = 50;  // current frame
    
    const SeekerObservation obs = buildSeekerObservation(state);
    
    // Test feature index 2 (current cpu)
    REQUIRE(obs.server.getOrDefault(0, {}).getOrDefault(2, int16_t{0}) == 128);  // 50 * 256 / 100 = 128
    
    // Test feature index 9 (cpu Δ1) 
    REQUIRE(obs.server.getOrDefault(0, {}).getOrDefault(9, int16_t{0}) == 51);   // (50 - 30) * 256 / 100 = 51
    
    // Test feature index 16 (cpu Δ2)
    REQUIRE(obs.server.getOrDefault(0, {}).getOrDefault(16, int16_t{0}) == 25);  // (50 - 40) * 256 / 100 = 25
}

TEST_CASE("FeatureExtractTest - Active but not connected server", "[feature-extract]") {
    MatchState state = {};
    
    // Set up an active but not connected server (server 1)
    state.servers[1].active = 1;
    state.seeker_connection_mask = 0b0;  // Not connected to any server
    
    // Set current metrics
    state.servers[1].current.cpu = 90;
    
    const SeekerObservation obs = buildSeekerObservation(state);
    
    // Test feature index 0 (active)
    REQUIRE(obs.server.getOrDefault(1, {}).getOrDefault(0, int16_t{0}) == 256);  // Server is active
    
    // Test feature index 1 (seeker connected) 
    REQUIRE(obs.server.getOrDefault(1, {}).getOrDefault(1, int16_t{0}) == 0);    // Not connected
    
    // Test that features 2-36 are all 0
    for (int i = 2; i <= 36; ++i) {
        REQUIRE(obs.server.getOrDefault(1, {}).getOrDefault(i, int16_t{0}) == 0);
    }
}

TEST_CASE("FeatureExtractTest - Inactive server", "[feature-extract]") {
    MatchState state = {};
    
    // Set up an inactive server (server 2)
    state.servers[2].active = 0;
    
    const SeekerObservation obs = buildSeekerObservation(state);
    
    // Test that all features for the inactive server are 0
    for (int i = 0; i < SEEKER_SERVER_FEATURES; ++i) {
        REQUIRE(obs.server.getOrDefault(2, {}).getOrDefault(i, int16_t{0}) == 0);
    }
}

TEST_CASE("FeatureExtractTest - Global features", "[feature-extract]") {
    MatchState state = {};
    
    // Set up global features
    state.seeker.connection_tokens = 12;
    state.seeker_connection_mask = 0b1;  // One open connection
    state.server_count = 2;  // Two servers total
    state.tick = 7;
    
    // Mark both servers as active
    state.servers[0].active = 1;
    state.servers[1].active = 1;
    
    const SeekerObservation obs = buildSeekerObservation(state);
    
    // Test global features
    REQUIRE(obs.global.getOrDefault(0, int16_t{0}) == 96);   // min(12, 32) * 8 = 96
    REQUIRE(obs.global.getOrDefault(1, int16_t{0}) == 16);   // popcount(1) * 16 = 16  
    REQUIRE(obs.global.getOrDefault(2, int16_t{0}) == 32);   // count_if(active) * 16 = 2 * 16 = 32
    REQUIRE(obs.global.getOrDefault(3, int16_t{0}) == 7);    // min(7, 256) = 7
}