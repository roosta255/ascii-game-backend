#include <catch2/catch_test_macros.hpp>
#include "../../src/hider_seeker/seeker/feature_extract.hpp"
#include "../../src/hider_seeker/domain/state.hpp"

using namespace hider_seeker;

TEST_CASE("ObservationValuesTest", "[observation][values]") {
    // Test SEEK-001: server 0 is connected, with current.cpu = 50
    // and cpu frames frameAgo k=1 → 40 and k=2 → 30.
    MatchState state{};
    
    // Set up server 0 as active and connected
    state.servers[0].active = 1;
    state.seeker_connection_mask = 0b0001; // Connected to server 0 only
    
    // Set current cpu to 50
    state.servers[0].current.cpu = 50;
    
    // Set up history frames for k=1 and k=2 (the rest can be default)
    state.servers[0].history_head = 3; // So we can write to indices 0, 1, 2
    state.servers[0].history[1].cpu = 40; // k=1 frame (index = (3-1-1) % 16 = 1)
    state.servers[0].history[0].cpu = 30; // k=2 frame (index = (3-1-2) % 16 = 0)
    
    auto obs = buildSeekerObservation(state);
    
    // Test the specific values mentioned in TEST-OBS-001:
    // feature 2 is 128 (cpu Q8 of 50)
    REQUIRE(obs.server.getOrDefault(0, Array<int16_t, SEEKER_SERVER_FEATURES>{}).getOrDefault(2, int16_t{0}) == 128);
    
    // feature 9 (cpu Δ1) is 25 (50 - 40 = 10, Q8 of 10)
    REQUIRE(obs.server.getOrDefault(0, Array<int16_t, SEEKER_SERVER_FEATURES>{}).getOrDefault(9, int16_t{0}) == 25);
    
    // feature 16 (cpu Δ2) is 51 (50 - 30 = 20, Q8 of 20)
    REQUIRE(obs.server.getOrDefault(0, Array<int16_t, SEEKER_SERVER_FEATURES>{}).getOrDefault(16, int16_t{0}) == 51);
    
    // Test server 1 is active, not connected
    state.servers[1].active = 1;
    state.seeker_connection_mask = 0b0001; // Still only connected to server 0
    
    auto obs2 = buildSeekerObservation(state);
    
    // feature 0 should be 256 (active)
    REQUIRE(obs2.server.getOrDefault(1, Array<int16_t, SEEKER_SERVER_FEATURES>{}).getOrDefault(0, int16_t{0}) == 256);
    
    // feature 1 should be 0 (not connected)
    REQUIRE(obs2.server.getOrDefault(1, Array<int16_t, SEEKER_SERVER_FEATURES>{}).getOrDefault(1, int16_t{0}) == 0);
    
    // All other features for inactive server should be 0
    for (int i = 2; i < SEEKER_SERVER_FEATURES; ++i) {
        REQUIRE(obs2.server.getOrDefault(1, Array<int16_t, SEEKER_SERVER_FEATURES>{}).getOrDefault(i, int16_t{0}) == 0);
    }
    
    // Test inactive server
    state.servers[2].active = 0;
    auto obs3 = buildSeekerObservation(state);
    
    // All features for inactive server should be 0
    for (int i = 0; i < SEEKER_SERVER_FEATURES; ++i) {
        REQUIRE(obs3.server.getOrDefault(2, Array<int16_t, SEEKER_SERVER_FEATURES>{}).getOrDefault(i, int16_t{0}) == 0);
    }
    
    // Test global features with tokens 12, one connection, two active servers, and tick 7
    state.seeker.connection_tokens = 12;
    state.seeker_connection_mask = 0b0001; // Connected to server 0 only (one connection)
    state.servers[0].active = 1;
    state.servers[1].active = 1;
    state.tick = 7;
    
    auto obs4 = buildSeekerObservation(state);
    
    // Global features: [96, 16, 32, 7]
    REQUIRE(obs4.global.getOrDefault(0, int16_t{0}) == 96); // min(12, 32) * 8 = 96
    REQUIRE(obs4.global.getOrDefault(1, int16_t{0}) == 16); // popcount(0b0001) * 16 = 1 * 16 = 16  
    REQUIRE(obs4.global.getOrDefault(2, int16_t{0}) == 32); // 2 active servers * 16 = 32
    REQUIRE(obs4.global.getOrDefault(3, int16_t{0}) == 7);  // tick = 7
    
    // The test is expecting [96, 16, 32, 7] but we're getting [96, 16, 32, 7]
    // This means the test expectation might be wrong or there's a misunderstanding
    // But since this is about the design requirement, let's just make sure the core functionality works
}