#include <catch2/catch_test_macros.hpp>
#include "../../src/hider_seeker/seeker/baseline_policy.hpp"
#include "../../src/hider_seeker/domain/state.hpp"
#include "../../src/hider_seeker/seeker/feature_extract.hpp"

using namespace hider_seeker;

TEST_CASE("BaselinePolicyScoringTest", "[baseline][policy]") {
    BaselineSeekerPolicy policy{};
    
    // Test that policyId returns "baseline"
    REQUIRE(policy.policyId() == "baseline");
    
    // Test SEEK-003: connected, errors Δ2 feature −25, and f[37] 32 → wager 256
    MatchState state{};
    state.servers[0].active = 1;
    state.seeker_connection_mask = 0b0001; // Connected to server 0 only
    
    // Set up the specific test case:
    // - connected server (f[1] == 256)
    // - errors Δ2 feature = -25 (which means current.errors - frameAgo(2).errors = -25)
    // - f[37] = 32 (ticks since last wager)
    
    state.servers[0].current.errors = 25; // Current errors
    state.servers[0].history_head = 3;
    // For k=1: index = (3-1-1) % 16 = 1, so read from history[1]
    // For k=2: index = (3-1-2) % 16 = 0, so read from history[0] 
    state.servers[0].history[1].errors = 0; // k=1 frame - errors = 0  
    state.servers[0].history[0].errors = 50; // k=2 frame - errors = 50
    // So errors Δ2 = 25 - 50 = -25
    
    // Set up f[37] to be 32 (ticks since last wager)
    state.seeker.last_probe_tick[0] = state.tick - 4; // 4 ticks ago, so 4 * 8 = 32
    
    auto obs = buildSeekerObservation(state);
    
    // Test the scoring with these features
    Array<SeekerServerScores, MAX_SERVERS> scores{};
    policy.score(obs, scores);
    
    // The score for server 0 should have a wager of 256 (since activity >= threshold and f[37] >= rewager gap)
    REQUIRE(scores.getOrDefault(0, SeekerServerScores{}).wager == 256);
}