#include <catch2/catch_test_macros.hpp>
#include "../../src/hider_seeker/seeker/feature_extract.hpp"
#include "../../src/hider_seeker/domain/state.hpp"
#include <cstring>

using namespace hider_seeker;

TEST_CASE("ObservationLeakTest", "[observation][leak]") {
    // This test verifies that buildSeekerObservation doesn't leak data
    // by mutating each unrelated MatchState field and asserting that
    // the resulting observation bytes are identical to the baseline.
    
    // Set up a baseline state with some active servers and connections
    MatchState baseline_state{};
    baseline_state.servers[0].active = 1;
    baseline_state.seeker_connection_mask = 0b0001; // Connected to server 0
    baseline_state.servers[0].current.cpu = 50;
    baseline_state.servers[0].history_head = 3;
    baseline_state.servers[0].history[1].cpu = 40; // k=1 frame
    baseline_state.servers[0].history[0].cpu = 30; // k=2 frame
    
    auto baseline_obs = buildSeekerObservation(baseline_state);
    
    // Test that changing trace_strength doesn't affect the observation
    MatchState state_with_trace_strength{};
    state_with_trace_strength.servers[0].active = 1;
    state_with_trace_strength.seeker_connection_mask = 0b0001;
    state_with_trace_strength.servers[0].current.cpu = 50;
    state_with_trace_strength.servers[0].history_head = 3;
    state_with_trace_strength.servers[0].history[1].cpu = 40;
    state_with_trace_strength.servers[0].history[0].cpu = 30;
    state_with_trace_strength.servers[0].trace_strength = 42; // Mutated field
    
    auto obs_with_trace_strength = buildSeekerObservation(state_with_trace_strength);
    REQUIRE(std::memcmp(&baseline_obs, &obs_with_trace_strength, sizeof(baseline_obs)) == 0);
    
    // Test that changing trace_age doesn't affect the observation
    MatchState state_with_trace_age{};
    state_with_trace_age.servers[0].active = 1;
    state_with_trace_age.seeker_connection_mask = 0b0001;
    state_with_trace_age.servers[0].current.cpu = 50;
    state_with_trace_age.servers[0].history_head = 3;
    state_with_trace_age.servers[0].history[1].cpu = 40;
    state_with_trace_age.servers[0].history[0].cpu = 30;
    state_with_trace_age.servers[0].trace_age = 42; // Mutated field
    
    auto obs_with_trace_age = buildSeekerObservation(state_with_trace_age);
    REQUIRE(std::memcmp(&baseline_obs, &obs_with_trace_age, sizeof(baseline_obs)) == 0);
    
    // Test that changing owner_hider_id doesn't affect the observation
    MatchState state_with_owner_hider_id{};
    state_with_owner_hider_id.servers[0].active = 1;
    state_with_owner_hider_id.seeker_connection_mask = 0b0001;
    state_with_owner_hider_id.servers[0].current.cpu = 50;
    state_with_owner_hider_id.servers[0].history_head = 3;
    state_with_owner_hider_id.servers[0].history[1].cpu = 40;
    state_with_owner_hider_id.servers[0].history[0].cpu = 30;
    state_with_owner_hider_id.servers[0].owner_hider_id = 42; // Mutated field
    
    auto obs_with_owner_hider_id = buildSeekerObservation(state_with_owner_hider_id);
    REQUIRE(std::memcmp(&baseline_obs, &obs_with_owner_hider_id, sizeof(baseline_obs)) == 0);
    
    // Test that changing public_flags doesn't affect the observation
    MatchState state_with_public_flags{};
    state_with_public_flags.servers[0].active = 1;
    state_with_public_flags.seeker_connection_mask = 0b0001;
    state_with_public_flags.servers[0].current.cpu = 50;
    state_with_public_flags.servers[0].history_head = 3;
    state_with_public_flags.servers[0].history[1].cpu = 40;
    state_with_public_flags.servers[0].history[0].cpu = 30;
    state_with_public_flags.servers[0].public_flags = 42; // Mutated field
    
    auto obs_with_public_flags = buildSeekerObservation(state_with_public_flags);
    REQUIRE(std::memcmp(&baseline_obs, &obs_with_public_flags, sizeof(baseline_obs)) == 0);
    
    // Test that changing hider's server_id doesn't affect the observation
    MatchState state_with_hider_server_id{};
    state_with_hider_server_id.servers[0].active = 1;
    state_with_hider_server_id.seeker_connection_mask = 0b0001;
    state_with_hider_server_id.servers[0].current.cpu = 50;
    state_with_hider_server_id.servers[0].history_head = 3;
    state_with_hider_server_id.servers[0].history[1].cpu = 40;
    state_with_hider_server_id.servers[0].history[0].cpu = 30;
    state_with_hider_server_id.hiders[0].server_id = 42; // Mutated field
    
    auto obs_with_hider_server_id = buildSeekerObservation(state_with_hider_server_id);
    REQUIRE(std::memcmp(&baseline_obs, &obs_with_hider_server_id, sizeof(baseline_obs)) == 0);
    
    // Test that changing hider's action_points doesn't affect the observation
    MatchState state_with_hider_ap{};
    state_with_hider_ap.servers[0].active = 1;
    state_with_hider_ap.seeker_connection_mask = 0b0001;
    state_with_hider_ap.servers[0].current.cpu = 50;
    state_with_hider_ap.servers[0].history_head = 3;
    state_with_hider_ap.servers[0].history[1].cpu = 40;
    state_with_hider_ap.servers[0].history[0].cpu = 30;
    state_with_hider_ap.hiders[0].action_points = 42; // Mutated field
    
    auto obs_with_hider_ap = buildSeekerObservation(state_with_hider_ap);
    REQUIRE(std::memcmp(&baseline_obs, &obs_with_hider_ap, sizeof(baseline_obs)) == 0);
    
    // Test that changing hider's resources doesn't affect the observation
    MatchState state_with_hider_resources{};
    state_with_hider_resources.servers[0].active = 1;
    state_with_hider_resources.seeker_connection_mask = 0b0001;
    state_with_hider_resources.servers[0].current.cpu = 50;
    state_with_hider_resources.servers[0].history_head = 3;
    state_with_hider_resources.servers[0].history[1].cpu = 40;
    state_with_hider_resources.servers[0].history[0].cpu = 30;
    state_with_hider_resources.hiders[0].resources[0] = 42; // Mutated field
    
    auto obs_with_hider_resources = buildSeekerObservation(state_with_hider_resources);
    REQUIRE(std::memcmp(&baseline_obs, &obs_with_hider_resources, sizeof(baseline_obs)) == 0);
    
    // Test that changing hider_connection_pool doesn't affect the observation
    MatchState state_with_hider_pool{};
    state_with_hider_pool.servers[0].active = 1;
    state_with_hider_pool.seeker_connection_mask = 0b0001;
    state_with_hider_pool.servers[0].current.cpu = 50;
    state_with_hider_pool.servers[0].history_head = 3;
    state_with_hider_pool.servers[0].history[1].cpu = 40;
    state_with_hider_pool.servers[0].history[0].cpu = 30;
    state_with_hider_pool.hider_connection_pool = 42; // Mutated field
    
    auto obs_with_hider_pool = buildSeekerObservation(state_with_hider_pool);
    REQUIRE(std::memcmp(&baseline_obs, &obs_with_hider_pool, sizeof(baseline_obs)) == 0);
    
    // Test that changing pending ring doesn't affect the observation
    MatchState state_with_pending_head{};
    state_with_pending_head.servers[0].active = 1;
    state_with_pending_head.seeker_connection_mask = 0b0001;
    state_with_pending_head.servers[0].current.cpu = 50;
    state_with_pending_head.servers[0].history_head = 3;
    state_with_pending_head.servers[0].history[1].cpu = 40;
    state_with_pending_head.servers[0].history[0].cpu = 30;
    state_with_pending_head.pending_head = 42; // Mutated field
    
    auto obs_with_pending_head = buildSeekerObservation(state_with_pending_head);
    REQUIRE(std::memcmp(&baseline_obs, &obs_with_pending_head, sizeof(baseline_obs)) == 0);
    
    // Test that changing rng_state doesn't affect the observation
    MatchState state_with_rng_state{};
    state_with_rng_state.servers[0].active = 1;
    state_with_rng_state.seeker_connection_mask = 0b0001;
    state_with_rng_state.servers[0].current.cpu = 50;
    state_with_rng_state.servers[0].history_head = 3;
    state_with_rng_state.servers[0].history[1].cpu = 40;
    state_with_rng_state.servers[0].history[0].cpu = 30;
    state_with_rng_state.rng_state = 42; // Mutated field
    
    auto obs_with_rng_state = buildSeekerObservation(state_with_rng_state);
    REQUIRE(std::memcmp(&baseline_obs, &obs_with_rng_state, sizeof(baseline_obs)) == 0);
    
    // Test that changing metrics of disconnected servers doesn't affect the observation
    MatchState state_with_disconnected_metrics{};
    state_with_disconnected_metrics.servers[0].active = 1;
    state_with_disconnected_metrics.seeker_connection_mask = 0b0001;
    state_with_disconnected_metrics.servers[0].current.cpu = 50;
    state_with_disconnected_metrics.servers[0].history_head = 3;
    state_with_disconnected_metrics.servers[0].history[1].cpu = 40;
    state_with_disconnected_metrics.servers[0].history[0].cpu = 30;
    state_with_disconnected_metrics.servers[1].current.cpu = 42; // Mutated field (disconnected server)
    
    auto obs_with_disconnected_metrics = buildSeekerObservation(state_with_disconnected_metrics);
    REQUIRE(std::memcmp(&baseline_obs, &obs_with_disconnected_metrics, sizeof(baseline_obs)) == 0);
    
    // Test that changing history of disconnected servers doesn't affect the observation
    MatchState state_with_disconnected_history{};
    state_with_disconnected_history.servers[0].active = 1;
    state_with_disconnected_history.seeker_connection_mask = 0b0001;
    state_with_disconnected_history.servers[0].current.cpu = 50;
    state_with_disconnected_history.servers[0].history_head = 3;
    state_with_disconnected_history.servers[0].history[1].cpu = 40;
    state_with_disconnected_history.servers[0].history[0].cpu = 30;
    state_with_disconnected_history.servers[1].history_head = 42; // Mutated field (disconnected server)
    
    auto obs_with_disconnected_history = buildSeekerObservation(state_with_disconnected_history);
    REQUIRE(std::memcmp(&baseline_obs, &obs_with_disconnected_history, sizeof(baseline_obs)) == 0);
}