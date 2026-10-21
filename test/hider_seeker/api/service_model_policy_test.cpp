#include <catch2/catch_test_macros.hpp>
#include "service_test_support.hpp"
#include "model_policy.hpp"
#include "seeker_model.hpp"
#include "HiderSeekerService.hpp"
#include "match_setup.hpp"
#include "serialization.hpp"
#include <nlohmann/json.hpp>
#include <filesystem>

using namespace hider_seeker;
using json = nlohmann::json;

// Add the missing using declarations from service_test_support
using hs_test::ServiceFixture;
using hs_test::intentOf;

TEST_CASE("End-to-end determinism with model policy", "[service_model_policy]") {
    // Construct ModelSeekerPolicy with a random model (seed 1) and version "r1"
    ModelSeekerPolicy policy(makeRandomSeekerModel(1), "r1");
    
    // Create two ServiceFixture instances with the same policy
    ServiceFixture a(policy);
    ServiceFixture b(policy);
    
    // Execute the same sequence on both fixtures
    a.setupTwoHiders();
    b.setupTwoHiders();
    
    // Play BOOST_CPU for alice
    auto alicePlayResponse = a.play("alice", intentOf("BOOST_CPU"));
    auto bobPassResponse = a.pass("bob", 4);
    auto alicePlayResponse2 = b.play("alice", intentOf("BOOST_CPU"));
    auto bobPassResponse2 = b.pass("bob", 4);
    
    // Verify all responses have status 200
    REQUIRE(alicePlayResponse.status == 200);
    REQUIRE(bobPassResponse.status == 200);
    REQUIRE(alicePlayResponse2.status == 200);
    REQUIRE(bobPassResponse2.status == 200);
    
    // Verify every request-log entry where kind == "move" has seeker_policy == "model:r1"
    for (const auto& entry : a.record().request_log) {
        if (entry.kind == "move") {
            REQUIRE(entry.body["seeker_policy"] == "model:r1");
        }
    }
    
    for (const auto& entry : b.record().request_log) {
        if (entry.kind == "move") {
            REQUIRE(entry.body["seeker_policy"] == "model:r1");
        }
    }
    
    // Compare public views
    auto aPublicView = a.service.getLatestView(a.matchId, "public", "", "");
    auto bPublicView = b.service.getLatestView(b.matchId, "public", "", "");
    
    REQUIRE(aPublicView.status == 200);
    REQUIRE(bPublicView.status == 200);
    
    // Compare hider views for alice
    auto aHiderView = a.service.getLatestView(a.matchId, "hider", "alice", "");
    auto bHiderView = b.service.getLatestView(b.matchId, "hider", "alice", "");
    
    REQUIRE(aHiderView.status == 200);
    REQUIRE(bHiderView.status == 200);
    
    // Remove match_id and content_hash from both views for comparison
    auto aPublicBody = aPublicView.body;
    auto bPublicBody = bPublicView.body;
    aPublicBody.erase("match_id");
    aPublicBody.erase("content_hash");
    bPublicBody.erase("match_id");
    bPublicBody.erase("content_hash");
    
    auto aHiderBody = aHiderView.body;
    auto bHiderBody = bHiderView.body;
    aHiderBody.erase("match_id");
    aHiderBody.erase("content_hash");
    bHiderBody.erase("match_id");
    bHiderBody.erase("content_hash");
    
    // Compare the views
    REQUIRE(aPublicBody.dump() == bPublicBody.dump());
    REQUIRE(aHiderBody.dump() == bHiderBody.dump());
    
    // Compare stored match states after serialization
    std::vector<uint8_t> aStateBytes, bStateBytes;
    saveMatchState(a.record().state, aStateBytes);
    saveMatchState(b.record().state, bStateBytes);
    
    REQUIRE(aStateBytes.size() == bStateBytes.size());
    for (size_t i = 0; i < aStateBytes.size(); ++i) {
        REQUIRE(aStateBytes[i] == bStateBytes[i]);
    }
}