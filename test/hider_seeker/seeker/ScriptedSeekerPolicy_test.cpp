#include <catch2/catch_test_macros.hpp>
#include "ScriptedSeekerPolicy.hpp"

using namespace hider_seeker;

TEST_CASE("SCRIPT-001: Scripted seeker policy behavior", "[hider_seeker][seeker][policy]") {
    ScriptedSeekerPolicy policy;
    
    // Test that policyId returns "scripted"
    REQUIRE(policy.policyId() == "scripted");
    
    // Set up some test scores
    policy.stored.access(0, [&](SeekerServerScores& score) {
        score.open = 128;
        score.close = 256;
        score.wager = 384;
    });
    
    // Create a dummy observation (not used by ScriptedSeekerPolicy)
    SeekerObservation obs = {};
    
    // Create output array
    Array<SeekerServerScores, MAX_SERVERS> scores;
    
    // Test that score method copies stored scores correctly
    policy.score(obs, scores);
    
    REQUIRE(scores.getOrDefault(0, SeekerServerScores{}).open == 128);
    REQUIRE(scores.getOrDefault(0, SeekerServerScores{}).close == 256);
    REQUIRE(scores.getOrDefault(0, SeekerServerScores{}).wager == 384);
    
    // Test with multiple servers
    policy.stored.access(1, [&](SeekerServerScores& score) {
        score.open = 512;
        score.close = 640;
        score.wager = 768;
    });
    
    policy.score(obs, scores);
    
    REQUIRE(scores.getOrDefault(0, SeekerServerScores{}).open == 128);
    REQUIRE(scores.getOrDefault(0, SeekerServerScores{}).close == 256);
    REQUIRE(scores.getOrDefault(0, SeekerServerScores{}).wager == 384);
    
    REQUIRE(scores.getOrDefault(1, SeekerServerScores{}).open == 512);
    REQUIRE(scores.getOrDefault(1, SeekerServerScores{}).close == 640);
    REQUIRE(scores.getOrDefault(1, SeekerServerScores{}).wager == 768);
}