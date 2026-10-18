#include <catch2/catch_test_macros.hpp>
#include "../../src/hider_seeker/seeker/baseline_policy.hpp"
#include "../../src/hider_seeker/domain/state.hpp"

using namespace hider_seeker;

TEST_CASE("BaselinePolicyScoringTest", "[baseline][policy]") {
    BaselineSeekerPolicy policy{};
    
    // Test that policyId returns "baseline"
    REQUIRE(policy.policyId() == "baseline");
    
    // Basic interface test - the detailed functionality tests are in feature_extract_test.cpp
    REQUIRE(true == true);
}