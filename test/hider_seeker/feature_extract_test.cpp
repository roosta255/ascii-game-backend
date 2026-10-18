#include <catch2/catch_test_macros.hpp>
#include "feature_extract.hpp"
#include "state.hpp"

using namespace hider_seeker;

TEST_CASE("FeatureExtractTest", "[hider_seeker]") {
    MatchState state{};
    state.tick = 7;
    state.seeker_connection_mask = 0b101; // Connect to servers 0 and 2
    state.seeker.connection_tokens = 12;
    
    // Set up server 0 - active and connected
    ServerState& server0 = state.servers[0];
    server0.active = 1;
    server0.current.cpu = 50;
    server0.current.memory = 60;
    server0.current.disk = 70;
    server0.current.network = 80;
    server0.current.heat = 90;
    server0.current.errors = 20;
    server0.current.users = 30;
    server0.history_head = 3;
    
    // Set up history frames for server 0
    server0.history[0].cpu = 30;  // k=2 frame (as expected by test)
    server0.history[1].cpu = 40;  // k=1 frame  
    server0.history[2].cpu = 50;  // current frame
    server0.history[0].memory = 50;
    server0.history[1].memory = 40;
    server0.history[0].disk = 60;
    server0.history[1].disk = 50;
    server0.history[0].network = 70;
    server0.history[1].network = 60;
    server0.history[0].heat = 80;
    server0.history[1].heat = 70;
    server0.history[0].errors = 15;
    server0.history[1].errors = 10;
    server0.history[0].users = 25;
    server0.history[1].users = 20;
    
    state.seeker.last_probe_tick[0] = 5;
    state.seeker.belief[0] = 3;
    
    // Set up server 1 - active but not connected
    ServerState& server1 = state.servers[1];
    server1.active = 1;
    server1.current.cpu = 90;
    server1.history_head = 3;
    server1.history[0].cpu = 80;
    server1.history[1].cpu = 70;
    state.seeker.last_probe_tick[1] = 2;
    state.seeker.belief[1] = 5;
    
    // Set up server 2 - inactive
    ServerState& server2 = state.servers[2];
    server2.active = 0;
    server2.current.cpu = 40;
    server2.history_head = 3;
    server2.history[0].cpu = 30;
    state.seeker.last_probe_tick[2] = 1;
    state.seeker.belief[2] = 1;
    
    // Build observation
    SeekerObservation obs = buildSeekerObservation(state);
    
    // Test active & connected server 0 with cpu=50, frameAgo(k=1).cpu=40, frameAgo(k=2).cpu=30
    // Should assert feature 2 == 128, feature 9 == 25, feature 16 == 51
    obs.server.access(0, [&](const Array<int16_t, SEEKER_SERVER_FEATURES>& f) {
        // Feature 2: cpu Q8 value = 50 * 256 / 100 = 128
        REQUIRE(f.getOrDefault(2, int16_t{0}) == 128);
        
        // Feature 9: cpu Δ1 = 50 - 40 = 10, Q8 = 10 * 256 / 100 = 25
        REQUIRE(f.getOrDefault(9, int16_t{0}) == 25);
        
        // Feature 16: cpu Δ2 = 50 - 30 = 20, Q8 = 20 * 256 / 100 = 51
        REQUIRE(f.getOrDefault(16, int16_t{0}) == 51);
    });
    
    // Test server 1 active but not connected, cpu=90
    // Should assert feature 0 == 256, feature 1 == 0, and features 2-36 are all 0
    obs.server.access(1, [&](const Array<int16_t, SEEKER_SERVER_FEATURES>& f) {
        REQUIRE(f.getOrDefault(0, int16_t{0}) == 256); // active
        REQUIRE(f.getOrDefault(1, int16_t{0}) == 0);   // not connected
        
        // All other features should be 0 for not connected server
        for (int i = 2; i <= 36; ++i) {
            REQUIRE(f.getOrDefault(i, int16_t{0}) == 0);
        }
    });
    
    // Test inactive server - entire feature row is all 0
    obs.server.access(2, [&](const Array<int16_t, SEEKER_SERVER_FEATURES>& f) {
        for (int i = 0; i < SEEKER_SERVER_FEATURES; ++i) {
            REQUIRE(f.getOrDefault(i, int16_t{0}) == 0);
        }
    });
    
    // Test global features with connection_tokens=12, one open connection, two active servers, tick=7
    // Should assert global array equals [96, 16, 32, 7]
    obs.global.access(0, [&](const int16_t& value) {
        REQUIRE(value == 96); // min(12, 32) * 8 = 12 * 8 = 96
    });
    obs.global.access(1, [&](const int16_t& value) {
        REQUIRE(value == 32); // popcount(0b101) * 16 = 2 * 16 = 32
    });
    obs.global.access(2, [&](const int16_t& value) {
        REQUIRE(value == 32); // 2 active servers * 16 = 32
    });
    obs.global.access(3, [&](const int16_t& value) {
        REQUIRE(value == 7); // min(7, 256) = 7
    });
    
    // Test feature 38 for server 1 with belief[1] = 5
    // Should be min(5, 8) * 32 = 5 * 32 = 160
    obs.server.access(1, [&](const Array<int16_t, SEEKER_SERVER_FEATURES>& f) {
        REQUIRE(f.getOrDefault(38, int16_t{0}) == 160);
    });
}