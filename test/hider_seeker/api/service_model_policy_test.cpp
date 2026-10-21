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
    // Create a temporary directory for testing
    std::string tempDir = "test_temp_" + std::to_string(std::time(nullptr));
    
    // Ensure the directory doesn't exist
    std::filesystem::remove_all(tempDir);
    std::filesystem::create_directories(tempDir);
    
    // Construct ModelSeekerPolicy with a random model (seed 1) and version "r1"
    ModelSeekerPolicy policy(makeRandomSeekerModel(1), "r1");
    
    // Create two ServiceFixture instances with the same policy
    ServiceFixture a(policy);
    ServiceFixture b(policy);
    
    // Execute the same sequence on both fixtures
    a.setupTwoHiders();
    b.setupTwoHiders();
    
    // Play BOOST_CPU for alice
    a.play("alice", intentOf("BOOST_CPU"));
    b.play("alice", intentOf("BOOST_CPU"));
    
    // Pass 4 ticks for bob
    a.pass("bob", 4);
    b.pass("bob", 4);
    
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
    
    // Compare stored match states after serialization
    std::vector<uint8_t> aStateBytes, bStateBytes;
    saveMatchState(a.record().state, aStateBytes);
    saveMatchState(b.record().state, bStateBytes);
    
    REQUIRE(aStateBytes.size() == bStateBytes.size());
    for (size_t i = 0; i < aStateBytes.size(); ++i) {
        REQUIRE(aStateBytes[i] == bStateBytes[i]);
    }
    
    // Clean up
    std::filesystem::remove_all(tempDir);
}