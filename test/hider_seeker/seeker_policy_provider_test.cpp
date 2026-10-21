#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "seeker_policy_provider.hpp"
#include "model_store.hpp"
#include "seeker_model.hpp"
#include "baseline_policy.hpp"
#include <filesystem>
#include <fstream>

using namespace hider_seeker;

TEST_CASE("SeekerPolicyProvider with empty root directory", "[seeker_policy_provider]") {
    // Create a temporary directory for testing
    std::string tempDir = "test_temp_" + std::to_string(std::time(nullptr));
    
    // Ensure the directory doesn't exist
    std::filesystem::remove_all(tempDir);
    std::filesystem::create_directories(tempDir);
    
    SeekerPolicyProvider provider(tempDir);
    
    // Should use baseline policy
    REQUIRE(provider.policy().policyId() == "baseline");
    REQUIRE(!provider.fallbackReason().empty());
    
    // Clean up
    std::filesystem::remove_all(tempDir);
}

TEST_CASE("SeekerPolicyProvider with valid model", "[seeker_policy_provider]") {
    // Create a temporary directory for testing
    std::string tempDir = "test_temp_" + std::to_string(std::time(nullptr));
    
    // Ensure the directory doesn't exist
    std::filesystem::remove_all(tempDir);
    std::filesystem::create_directories(tempDir);
    
    // Write and activate a model version "r1" with seed 1
    SeekerModel model = makeRandomSeekerModel(1);
    std::string error;
    bool success = writeSeekerModel(tempDir, "r1", model, 1, true, error);
    REQUIRE(success);
    
    SeekerPolicyProvider provider(tempDir);
    
    // Should use the loaded model policy
    REQUIRE(provider.policy().policyId() == "model:r1");
    REQUIRE(provider.fallbackReason().empty());
    
    // Clean up
    std::filesystem::remove_all(tempDir);
}

TEST_CASE("SeekerPolicyProvider with corrupted model", "[seeker_policy_provider]") {
    // Create a temporary directory for testing
    std::string tempDir = "test_temp_" + std::to_string(std::time(nullptr));
    
    // Ensure the directory doesn't exist
    std::filesystem::remove_all(tempDir);
    std::filesystem::create_directories(tempDir);
    
    // Write and activate a model version "r1" with seed 1
    SeekerModel model = makeRandomSeekerModel(1);
    std::string error;
    bool success = writeSeekerModel(tempDir, "r1", model, 1, true, error);
    REQUIRE(success);
    
    // Corrupt one byte of weights.bin
    std::filesystem::path weightsPath = tempDir + "/r1/weights.bin";
    std::ifstream file(weightsPath, std::ios::binary);
    if (file.is_open()) {
        std::vector<char> buffer(std::filesystem::file_size(weightsPath));
        file.read(buffer.data(), buffer.size());
        file.close();
        
        // Flip one bit in the first byte
        if (!buffer.empty()) {
            buffer[0] ^= 0xFF;
        }
        
        std::ofstream outFile(weightsPath, std::ios::binary);
        outFile.write(buffer.data(), buffer.size());
        outFile.close();
    }
    
    SeekerPolicyProvider provider(tempDir);
    
    // Should fall back to baseline policy
    REQUIRE(provider.policy().policyId() == "baseline");
    REQUIRE(provider.fallbackReason().find("checksum") != std::string::npos);
    
    // Clean up
    std::filesystem::remove_all(tempDir);
}