#include <catch2/catch_test_macros.hpp>
#include "service_test_support.hpp"
#include "null_seeker_policy.hpp"
#include "baseline_policy.hpp"

using namespace hs_test;
using namespace hider_seeker;

TEST_CASE("ServiceSeekerPolicyTest: request log records null policy", "[service][policy]") {
    ServiceFixture fixture;
    
    // Create a match with default NullSeekerPolicy
    fixture.setupTwoHiders();
    
    // Play a move to trigger a committed action
    auto response = fixture.play("alice", intentOf("BOOST_CPU"));
    REQUIRE(response.status == 200);
    
    // Check that the request log contains the seeker_policy field
    auto record = fixture.record();
    REQUIRE(record.request_log.size() >= 1);
    auto& logEntry = record.request_log.back();
    REQUIRE(logEntry.body.contains("seeker_policy"));
    REQUIRE(logEntry.body["seeker_policy"] == "null");
}

TEST_CASE("ServiceSeekerPolicyTest: request log records baseline policy", "[service][policy]") {
    BaselineSeekerPolicy baselinePolicy;
    ServiceFixture fixture(baselinePolicy);
    
    // Create a match with BaselineSeekerPolicy
    fixture.setupTwoHiders();
    
    // Play a move to trigger a committed action
    auto response = fixture.play("alice", intentOf("BOOST_CPU"));
    REQUIRE(response.status == 200);
    
    // Check that the request log contains the seeker_policy field
    auto record = fixture.record();
    REQUIRE(record.request_log.size() >= 1);
    auto& logEntry = record.request_log.back();
    REQUIRE(logEntry.body.contains("seeker_policy"));
    REQUIRE(logEntry.body["seeker_policy"] == "baseline");
}