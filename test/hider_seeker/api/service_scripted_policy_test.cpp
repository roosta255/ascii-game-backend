#include <catch2/catch_test_macros.hpp>
#include "service_test_support.hpp"
#include "../ScriptedSeekerPolicy.hpp"

using namespace hs_test;
using namespace hider_seeker;

TEST_CASE("ServiceScriptedPolicyTest: end-to-end match end through API", "[service][policy][scripted]") {
    ScriptedSeekerPolicy policy;
    policy.scores.assignValue(0, SeekerServerScores{256, 0, 256}); // open 256, close 0, wager 256
    ServiceFixture fixture(policy);
    
    // Setup two hiders (alice on server 0, bob on server 1)
    fixture.setupTwoHiders();
    
    // Bob disconnects
    auto response = fixture.play("bob", intentOf("DISCONNECT"));
    REQUIRE(response.status == 200);
    
    // Load the stored match record and set hider_connection_pool to 0
    auto record = fixture.record();
    record.state.hider_connection_pool = 0;
    fixture.writeRecord(record);
    
    // Bob passes 5 ticks (this should advance exactly 2 ticks)
    response = fixture.pass("bob", 5);
    REQUIRE(response.status == 200);
    
    // Check that exactly 2 ticks have been advanced
    REQUIRE(fixture.tick() == 2);
    
    // Attempt another move - should be rejected with MATCH_NOT_ACTIVE
    response = fixture.pass("bob", 1);
    REQUIRE(response.status == 422);
    REQUIRE(response.body["rule_outcome"] == "MATCH_NOT_ACTIVE");
    
    // Check both public and hider views contain correct phase and winner
    auto publicView = fixture.service.getLatestView(fixture.matchId, "public", "", "");
    REQUIRE(publicView.body["phase"] == "COMPLETE");
    REQUIRE(publicView.body["winner"] == "SEEKER");
    
    // Get alice's view (hider view)
    auto aliceView = fixture.service.getLatestView(fixture.matchId, "hider", "alice", "");
    REQUIRE(aliceView.body["phase"] == "COMPLETE");
    REQUIRE(aliceView.body["winner"] == "SEEKER");
}