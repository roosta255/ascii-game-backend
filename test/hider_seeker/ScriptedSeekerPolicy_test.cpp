#include <catch2/catch_test_macros.hpp>
#include "../hider_seeker/seeker/policy_interface.hpp"
#include "test/hider_seeker/ScriptedSeekerPolicy.hpp"

using namespace hider_seeker;

TEST_CASE("ScriptedSeekerPolicyTest", "[hider_seeker]") {
    ScriptedSeekerPolicy policy;
    
    // Test that policyId returns "scripted"
    REQUIRE(policy.policyId() == "scripted");
    
    // Test that score copies stored scores
    SeekerObservation obs{};
    Array<SeekerServerScores, MAX_SERVERS> outputScores;
    
    // Set some test scores
    policy.scores.access(0, [&](SeekerServerScores& s) {
        s.open = 128;
        s.close = 256;
        s.wager = 384;
    });
    
    // Call score method
    policy.score(obs, outputScores);
    
    // Verify scores were copied
    outputScores.access(0, [&](const SeekerServerScores& s) {
        REQUIRE(s.open == 128);
        REQUIRE(s.close == 256);
        REQUIRE(s.wager == 384);
    });
}