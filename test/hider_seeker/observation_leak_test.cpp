#include <catch2/catch_test_macros.hpp>
#include "state.hpp"
#include "feature_extract.hpp"
#include "ScriptedSeekerPolicy.hpp"

using namespace hider_seeker;

TEST_CASE("ObservationLeakTest", "[observation-leak]") {
    MatchState state = {};
    
    // Set up a basic match state with some data
    state.tick = 100;
    state.seeker_connection_mask = 0b101;  // Connected to servers 0 and 2
    state.seeker.connection_tokens = 15;
    
    // Initialize server 0 as active and connected
    state.servers[0].active = 1;
    state.servers[0].current.cpu = 50;
    state.servers[0].current.memory = 60;
    state.servers[0].current.disk = 70;
    state.servers[0].current.network = 80;
    state.servers[0].current.heat = 90;
    state.servers[0].current.errors = 5;
    state.servers[0].current.users = 10;
    
    // Set up history for server 0
    state.servers[0].history_head = 1;
    state.servers[0].history[0].cpu = 40;
    state.servers[0].history[0].memory = 50;
    state.servers[0].history[0].disk = 60;
    state.servers[0].history[0].network = 70;
    state.servers[0].history[0].heat = 80;
    state.servers[0].history[0].errors = 10;
    state.servers[0].history[0].users = 5;
    
    // Set up server 1 as active but not connected
    state.servers[1].active = 1;
    state.servers[1].current.cpu = 90;
    
    // Set up server 2 as inactive
    state.servers[2].active = 0;
    
    // Build the original observation
    const SeekerObservation originalObs = buildSeekerObservation(state);
    
    // Test that mutating unrelated fields doesn't change the observation
    
    // Mutate trace_strength (should not affect observation)
    state.servers[0].trace_strength = 42;
    
    // Mutate trace_age (should not affect observation)
    state.servers[0].trace_age = 15;
    
    // Mutate owner_hider_id (should not affect observation)
    state.servers[0].owner_hider_id = 3;
    
    // Mutate public_flags (should not affect observation)
    state.servers[0].public_flags = 7;
    
    // Mutate hider's server_id (should not affect observation)
    state.hiders[0].server_id = 5;
    
    // Mutate hider's action_points (should not affect observation)
    state.hiders[0].action_points = 20;
    
    // Mutate hider's resources (should not affect observation)
    state.hiders[0].resources[0] = 100;
    
    // Mutate hider_connection_pool (should not affect observation)
    state.hider_connection_pool = 5;
    
    // Mutate pending move ring (should not affect observation)
    state.pending_head = 3;
    state.pending[0].hider_id = 1;
    
    // Mutate rng_state (should not affect observation)
    state.rng_state = 123456789;
    
    // Mutate metrics and history of a server the seeker is NOT connected to
    state.servers[1].current.cpu = 95;
    state.servers[1].history[0].cpu = 85;
    
    // Build the new observation after mutations
    const SeekerObservation mutatedObs = buildSeekerObservation(state);
    
    // Compare byte-by-byte - they should be identical
    REQUIRE(memcmp(&originalObs, &mutatedObs, sizeof(SeekerObservation)) == 0);
}