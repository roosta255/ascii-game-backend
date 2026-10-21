#include <catch2/catch_test_macros.hpp>
#include "seeker_model_init.hpp"
#include "model_store.hpp"
#include "seeker_model.hpp"
#include <sstream>
#include <filesystem>

using namespace hider_seeker;

TEST_CASE("seeker_model_init CLI tests", "[seeker_model_init]") {
    // Create a temporary directory for testing
    std::string tempDir = "test_temp_" + std::to_string(std::time(nullptr));
    
    // Ensure the directory doesn't exist
    std::filesystem::remove_all(tempDir);
    std::filesystem::create_directories(tempDir);
    
    std::ostringstream out;
    std::ostringstream err;
    
    // Test 1: Successful creation with --activate flag
    {
        std::vector<std::string> args = {"--root", tempDir, "--version", "r1", "--seed", "1", "--activate"};
        int result = runSeekerModelInit(args, out, err);
        REQUIRE(result == 0);
        
        // Verify the model was actually created and loaded
        SeekerModel model;
        std::string version;
        std::string error;
        bool loaded = loadActiveSeekerModel(tempDir, model, version, error);
        REQUIRE(loaded);
        REQUIRE(version == "r1");
    }
    
    // Test 2: Attempt to create the same version again (should fail)
    {
        std::vector<std::string> args = {"--root", tempDir, "--version", "r1", "--seed", "1", "--activate"};
        int result = runSeekerModelInit(args, out, err);
        REQUIRE(result != 0);  // Should return non-zero
        REQUIRE(!err.str().empty());  // Should write an error message
    }
    
    // Test 3: Missing --version option (should return 2)
    {
        std::vector<std::string> args = {"--root", tempDir, "--seed", "1"};
        int result = runSeekerModelInit(args, out, err);
        REQUIRE(result == 2);  // Should return 2 for bad arguments
    }
    
    // Clean up
    std::filesystem::remove_all(tempDir);
}