#include <catch2/catch_test_macros.hpp>
#include "feature_extract.hpp"
#include "feature_schema.hpp"
#include "../../../src/hider_seeker/domain/state.hpp"

using namespace hider_seeker;

TEST_CASE("OBS-001: Feature values for connected server", "[hider_seeker][seeker][observation]") {
    MatchState state = {};
    
    // Set up server 0 as active and connected
    state.servers[0].active = 1;
    state.seeker_connection_mask = 1; // Connected to server 0
    
    // Set current metrics for server 0
    state.servers[0].current.cpu = 50;
    state.servers[0].current.memory = 60;
    state.servers[0].current.disk = 70;
    state.servers[0].current.network = 80;
    state.servers[0].current.heat = 90;
    state.servers[0].current.errors = 10;
    state.servers[0].current.users = 20;
    
    // Set up history for server 0 (frameAgo k=1 and k=2)
    state.servers[0].history_head = 3; // Points to index 2 for newest frame
    state.servers[0].history[0] = {40, 50, 60, 70, 80, 5, 15}; // k=1 frame (1 tick ago)
    state.servers[0].history[1] = {30, 40, 50, 60, 70, 2, 10}; // k=2 frame (2 ticks ago)
    state.servers[0].history[2] = {20, 30, 40, 50, 60, 1, 5};  // newest frame
    
    // Set tick and last probe tick
    state.tick = 10;
    state.seeker.last_probe_tick[0] = 5; // 5 ticks ago
    
    // Set seeker belief
    state.seeker.belief[0] = 3;
    
    SeekerObservation obs = buildSeekerObservation(state);
    
    // Test feature index 2 (cpu Q8) = 128 (50 * 256 / 100)
    REQUIRE(obs.server.getOrDefault(0, Array<int16_t, SEEKER_SERVER_FEATURES>{}).getOrDefault(2, int16_t{0}) == 128);
    
    // Test feature index 9 (cpu Δ1) = 25 (50 - 40 = 10, 10 * 256 / 100 = 25)
    REQUIRE(obs.server.getOrDefault(0, Array<int16_t, SEEKER_SERVER_FEATURES>{}).getOrDefault(9, int16_t{0}) == 25);
    
    // Test feature index 16 (cpu Δ2) = 51 (50 - 30 = 20, 20 * 256 / 100 = 51)
    REQUIRE(obs.server.getOrDefault(0, Array<int16_t, SEEKER_SERVER_FEATURES>{}).getOrDefault(16, int16_t{0}) == 51);
}

TEST_CASE("OBS-002: Feature values for disconnected server", "[hider_seeker][seeker][observation]") {
    MatchState state = {};
    
    // Set up server 1 as active but not connected
    state.servers[1].active = 1;
    
    // Set current metrics for server 1
    state.servers[1].current.cpu = 90;
    
    // Set tick and last probe tick
    state.tick = 10;
    state.seeker.last_probe_tick[1] = 5; // 5 ticks ago
    
    SeekerObservation obs = buildSeekerObservation(state);
    
    // Test feature index 0 (active) = 256
    REQUIRE(obs.server.getOrDefault(1, Array<int16_t, SEEKER_SERVER_FEATURES>{}).getOrDefault(0, int16_t{0}) == 256);
    
    // Test feature index 1 (seeker connected) = 0 (not connected)
    REQUIRE(obs.server.getOrDefault(1, Array<int16_t, SEEKER_SERVER_FEATURES>{}).getOrDefault(1, int16_t{0}) == 0);
    
    // Test that features 2-36 are all 0 (not connected)
    for (int i = 2; i <= 36; ++i) {
        REQUIRE(obs.server.getOrDefault(1, Array<int16_t, SEEKER_SERVER_FEATURES>{}).getOrDefault(i, int16_t{0}) == 0);
    }
}

TEST_CASE("OBS-003: Inactive server features are all zero", "[hider_seeker][seeker][observation]") {
    MatchState state = {};
    
    // Set up server 0 as inactive
    state.servers[0].active = 0;
    
    SeekerObservation obs = buildSeekerObservation(state);
    
    // Test that all features for inactive server are 0
    const auto& serverFeatures = obs.server.getOrDefault(0, Array<int16_t, SEEKER_SERVER_FEATURES>{});
    for (int i = 0; i < SEEKER_SERVER_FEATURES; ++i) {
        REQUIRE(serverFeatures.getOrDefault(i, int16_t{0}) == 0);
    }
}

TEST_CASE("OBS-004: Global features computation", "[hider_seeker][seeker][observation]") {
    MatchState state = {};
    
    // Set up global features
    state.seeker.connection_tokens = 12;
    state.seeker_connection_mask = 1; // One connection (server 0)
    state.server_count = 2;
    state.tick = 7;
    
    SeekerObservation obs = buildSeekerObservation(state);
    
    // Test global features
    REQUIRE(obs.global.getOrDefault(0, int16_t{0}) == 96);  // min(12, 32) * 8 = 12 * 8 = 96
    REQUIRE(obs.global.getOrDefault(1, int16_t{0}) == 16);  // popcount(1) * 16 = 1 * 16 = 16
    REQUIRE(obs.global.getOrDefault(2, int16_t{0}) == 32);  // 2 * 16 = 32
    REQUIRE(obs.global.getOrDefault(3, int16_t{0}) == 7);   // min(7, 256) = 7
}

TEST_CASE("OBS-005: Observation does not leak unrelated state", "[hider_seeker][seeker][observation]") {
    MatchState state1 = {};
    MatchState state2 = {};
    
    // Set up identical states except for fields that should not affect observation
    state1.servers[0].active = 1;
    state1.seeker_connection_mask = 1; // Connected to server 0
    
    // Set current metrics for server 0
    state1.servers[0].current.cpu = 50;
    state1.servers[0].history_head = 3;
    state1.servers[0].history[0] = {40, 50, 60, 70, 80, 5, 15};
    state1.servers[0].history[1] = {30, 40, 50, 60, 70, 2, 10};
    state1.servers[0].history[2] = {20, 30, 40, 50, 60, 1, 5};
    
    // Set tick and last probe tick
    state1.tick = 10;
    state1.seeker.last_probe_tick[0] = 5;
    state1.seeker.belief[0] = 3;
    
    // Copy the first state to second state
    state2 = state1;
    
    // Mutate fields that should not affect observation
    state2.trace_strength = 42;
    state2.trace_age = 100;
    state2.owner_hider_id = 5;
    state2.public_flags = 7;
    state2.hiders[0].server_id = 3;
    state2.hiders[0].action_points = 20;
    state2.hiders[0].resources[0] = 100;
    state2.hider_connection_pool = 15;
    state2.pending_head = 5;
    state2.rng_state = 999999;
    state2.servers[1].current.cpu = 80; // A server the seeker is not connected to
    state2.servers[1].history[0] = {70, 60, 50, 40, 30, 20, 10}; // History of a disconnected server
    
    SeekerObservation obs1 = buildSeekerObservation(state1);
    SeekerObservation obs2 = buildSeekerObservation(state2);
    
    // Verify byte-identical output
    const auto& serverFeatures1 = obs1.server.getOrDefault(0, Array<int16_t, SEEKER_SERVER_FEATURES>{});
    const auto& serverFeatures2 = obs2.server.getOrDefault(0, Array<int16_t, SEEKER_SERVER_FEATURES>{});
    
    // Compare all features
    for (int i = 0; i < SEEKER_SERVER_FEATURES; ++i) {
        REQUIRE(serverFeatures1.getOrDefault(i, int16_t{0}) == serverFeatures2.getOrDefault(i, int16_t{0}));
    }
    
    // Compare global features
    for (int i = 0; i < SEEKER_GLOBAL_FEATURES; ++i) {
        REQUIRE(obs1.global.getOrDefault(i, int16_t{0}) == obs2.global.getOrDefault(i, int16_t{0}));
    }
}