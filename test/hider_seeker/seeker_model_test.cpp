#include <catch2/catch_test_macros.hpp>
#include "seeker_model.hpp"
#include "baseline_policy.hpp"
#include "feature_schema.hpp"

using namespace hider_seeker;

TEST_CASE("FloorDivTests", "[seeker_model]") {
    REQUIRE(floorDiv(-257, 256) == -2);
    REQUIRE(floorDiv(-256, 256) == -1);
    REQUIRE(floorDiv(255, 256) == 0);
}

TEST_CASE("RandomInitAndByteLayoutTests", "[seeker_model]") {
    // Test that same seed produces identical models
    SeekerModel model1 = makeRandomSeekerModel(1);
    SeekerModel model2 = makeRandomSeekerModel(1);
    
    std::string bytes1 = seekerModelBytes(model1);
    std::string bytes2 = seekerModelBytes(model2);
    
    REQUIRE(bytes1 == bytes2);
    REQUIRE(bytes1.size() == SEEKER_MODEL_BYTES);
    
    // Test that different seed produces different models
    SeekerModel model3 = makeRandomSeekerModel(2);
    std::string bytes3 = seekerModelBytes(model3);
    
    REQUIRE(bytes1 != bytes3);
    
    // Test round-trip serialization
    std::string bytes = seekerModelBytes(model1);
    SeekerModel modelFromBytes;
    bool success = seekerModelFromBytes(bytes, modelFromBytes);
    
    REQUIRE(success == true);
    
    // Compare the bytes of original and deserialized models
    std::string bytesFromBytes = seekerModelBytes(modelFromBytes);
    REQUIRE(bytes == bytesFromBytes);
    
    // Test that invalid size returns false
    std::string shortBytes(SEEKER_MODEL_BYTES - 1, 'x');
    SeekerModel modelInvalid;
    success = seekerModelFromBytes(shortBytes, modelInvalid);
    
    REQUIRE(success == false);
}

TEST_CASE("ExistingTestsUnaffected", "[seeker_model]") {
    // Just verify that basic compilation works with existing code
    BaselineSeekerPolicy baselinePolicy;
    REQUIRE(baselinePolicy.policyId() == "baseline");
}