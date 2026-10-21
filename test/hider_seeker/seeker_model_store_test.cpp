#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include "model_store.hpp"
#include "seeker_model.hpp"
#include "hider_seeker/api/service_test_support.hpp"

using namespace hider_seeker;

TEST_CASE("SeekerModelStoreTests", "[seeker_model_store]") {
    // Use test support to get a temporary directory
    auto tempDir = std::filesystem::temp_directory_path() / "seeker_model_store_test";
    std::filesystem::create_directories(tempDir);
    
    // Clean up after the test
    std::filesystem::remove_all(tempDir);
    std::filesystem::create_directories(tempDir);
    
    std::string root = tempDir.string();
    std::string error;
    SeekerModel model;
    std::string version;
    
    // Test 1: Basic write and load round-trip
    SECTION("BasicWriteAndLoad") {
        model = makeRandomSeekerModel(1);
        
        // Write the model
        bool success = writeSeekerModel(root, "r1", model, 1, true, error);
        REQUIRE(success == true);
        REQUIRE(error.empty() == true);
        
        // Load the model back
        SeekerModel loadedModel;
        std::string loadedVersion;
        success = loadActiveSeekerModel(root, loadedModel, loadedVersion, error);
        REQUIRE(success == true);
        REQUIRE(error.empty() == true);
        REQUIRE(loadedVersion == "r1");
        
        // Verify that the bytes match
        std::string originalBytes = seekerModelBytes(model);
        std::string loadedBytes = seekerModelBytes(loadedModel);
        REQUIRE(originalBytes == loadedBytes);
        
        // Verify weights.bin exists and is exactly 4076 bytes
        std::string weightsPath = root + "/r1/weights.bin";
        REQUIRE(std::filesystem::exists(weightsPath));
        REQUIRE(std::filesystem::file_size(weightsPath) == SEEKER_MODEL_BYTES);
        
        // Verify manifest fields
        std::string manifestPath = root + "/r1/manifest.json";
        REQUIRE(std::filesystem::exists(manifestPath));
        
        std::ifstream manifestFile(manifestPath);
        std::string manifestContent((std::istreambuf_iterator<char>(manifestFile)),
                                    std::istreambuf_iterator<char>());
        manifestFile.close();
        
        // Parse the JSON to verify fields
        auto j = nlohmann::json::parse(manifestContent, nullptr, false);
        REQUIRE(j.value("format", "") == "hider-seeker-seeker-mlp");
        REQUIRE(j.value("format_version", 0) == 1);
        REQUIRE(j.value("model_version", "") == "r1");
        REQUIRE(j.value("feature_version", 0) == SEEKER_FEATURE_VERSION);
        REQUIRE(j.value("fixed_point_shift", 0) == 8);
        REQUIRE(j["layers"].size() == 4);
        REQUIRE(j["layers"][0] == 43);
        REQUIRE(j["layers"][1] == 32);
        REQUIRE(j["layers"][2] == 16);
        REQUIRE(j["layers"][3] == 3);
        REQUIRE(j.value("seed", 0) == 1);
        
        // Verify the checksum matches
        std::string computedChecksum = fnv1a64Hex(originalBytes);
        REQUIRE(j.value("weights_fnv1a64", "") == computedChecksum);
    }
    
    // Test 2: Error handling - single byte flip in weights.bin
    SECTION("ChecksumError") {
        model = makeRandomSeekerModel(1);
        
        // Write the model first
        bool success = writeSeekerModel(root, "r2", model, 2, true, error);
        REQUIRE(success == true);
        REQUIRE(error.empty() == true);
        
        // Corrupt one byte in weights.bin
        std::string weightsPath = root + "/r2/weights.bin";
        std::ifstream weightsFile(weightsPath, std::ios::binary);
        std::string weightBytes((std::istreambuf_iterator<char>(weightsFile)),
                                std::istreambuf_iterator<char>());
        weightsFile.close();
        
        // Flip a single byte
        if (!weightBytes.empty()) {
            weightBytes[10] ^= 0xFF;  // Flip byte at position 10
        }
        
        // Write back the corrupted bytes
        std::ofstream corruptedFile(weightsPath, std::ios::binary);
        corruptedFile.write(weightBytes.c_str(), weightBytes.size());
        corruptedFile.close();
        
        // Try to load - should fail with checksum error
        SeekerModel loadedModel;
        std::string loadedVersion;
        success = loadActiveSeekerModel(root, loadedModel, loadedVersion, error);
        REQUIRE(success == false);
        REQUIRE(error.find("checksum") != std::string::npos);
    }
    
    // Test 3: Error handling - truncated weights.bin
    SECTION("SizeError") {
        model = makeRandomSeekerModel(1);
        
        // Write the model first
        bool success = writeSeekerModel(root, "r3", model, 3, true, error);
        REQUIRE(success == true);
        REQUIRE(error.empty() == true);
        
        // Truncate weights.bin
        std::string weightsPath = root + "/r3/weights.bin";
        std::ofstream truncatedFile(weightsPath, std::ios::binary | std::ios::trunc);
        truncatedFile.close();
        
        // Try to load - should fail with size error
        SeekerModel loadedModel;
        std::string loadedVersion;
        success = loadActiveSeekerModel(root, loadedModel, loadedVersion, error);
        REQUIRE(success == false);
        REQUIRE(error.find("size") != std::string::npos);
    }
    
    // Test 4: Error handling - manifest with wrong layers
    SECTION("LayersError") {
        model = makeRandomSeekerModel(1);
        
        // Write the model first
        bool success = writeSeekerModel(root, "r4", model, 4, true, error);
        REQUIRE(success == true);
        REQUIRE(error.empty() == true);
        
        // Modify manifest.json to have wrong layers
        std::string manifestPath = root + "/r4/manifest.json";
        std::ifstream manifestFile(manifestPath);
        std::string manifestContent((std::istreambuf_iterator<char>(manifestFile)),
                                    std::istreambuf_iterator<char>());
        manifestFile.close();
        
        auto j = nlohmann::json::parse(manifestContent, nullptr, false);
        j["layers"] = {43, 32, 3};  // Wrong layers
        std::ofstream modifiedManifest(manifestPath);
        modifiedManifest << j.dump(2) << "\n";
        modifiedManifest.close();
        
        // Try to load - should fail with layers error
        SeekerModel loadedModel;
        std::string loadedVersion;
        success = loadActiveSeekerModel(root, loadedModel, loadedVersion, error);
        REQUIRE(success == false);
        REQUIRE(error.find("layers") != std::string::npos);
    }
    
    // Test 5: Error handling - manifest with wrong feature_version
    SECTION("FeatureVersionError") {
        model = makeRandomSeekerModel(1);
        
        // Write the model first
        bool success = writeSeekerModel(root, "r5", model, 5, true, error);
        REQUIRE(success == true);
        REQUIRE(error.empty() == true);
        
        // Modify manifest.json to have wrong feature_version
        std::string manifestPath = root + "/r5/manifest.json";
        std::ifstream manifestFile(manifestPath);
        std::string manifestContent((std::istreambuf_iterator<char>(manifestFile)),
                                    std::istreambuf_iterator<char>());
        manifestFile.close();
        
        auto j = nlohmann::json::parse(manifestContent, nullptr, false);
        j["feature_version"] = 2;  // Wrong feature version
        std::ofstream modifiedManifest(manifestPath);
        modifiedManifest << j.dump(2) << "\n";
        modifiedManifest.close();
        
        // Try to load - should fail with feature_version error
        SeekerModel loadedModel;
        std::string loadedVersion;
        success = loadActiveSeekerModel(root, loadedModel, loadedVersion, error);
        REQUIRE(success == false);
        REQUIRE(error.find("feature_version") != std::string::npos);
    }
    
    // Test 6: Error handling - missing ACTIVE file
    SECTION("MissingActiveError") {
        model = makeRandomSeekerModel(1);
        
        // Write the model first
        bool success = writeSeekerModel(root, "r6", model, 6, true, error);
        REQUIRE(success == true);
        REQUIRE(error.empty() == true);
        
        // Remove ACTIVE file
        std::string activePath = root + "/ACTIVE";
        std::filesystem::remove(activePath);
        
        // Try to load - should fail with ACTIVE error
        SeekerModel loadedModel;
        std::string loadedVersion;
        success = loadActiveSeekerModel(root, loadedModel, loadedVersion, error);
        REQUIRE(success == false);
        REQUIRE(error.find("ACTIVE") != std::string::npos);
    }
    
    // Test 7: Error handling - ACTIVE naming missing directory
    SECTION("MissingDirectoryError") {
        model = makeRandomSeekerModel(1);
        
        // Write the model first
        bool success = writeSeekerModel(root, "r7", model, 7, true, error);
        REQUIRE(success == true);
        REQUIRE(error.empty() == true);
        
        // Modify ACTIVE to point to a non-existent directory
        std::string activePath = root + "/ACTIVE";
        std::ofstream activeFile(activePath);
        activeFile << "nonexistent_version\n";
        activeFile.close();
        
        // Try to load - should fail with missing error
        SeekerModel loadedModel;
        std::string loadedVersion;
        success = loadActiveSeekerModel(root, loadedModel, loadedVersion, error);
        REQUIRE(success == false);
        REQUIRE(error.find("missing") != std::string::npos);
    }
    
    // Test 8: Duplicate write rejection
    SECTION("DuplicateWriteRejection") {
        model = makeRandomSeekerModel(1);
        
        // Write the model first
        bool success = writeSeekerModel(root, "r8", model, 8, true, error);
        REQUIRE(success == true);
        REQUIRE(error.empty() == true);
        
        // Store original files content for comparison
        std::string weightsPath = root + "/r8/weights.bin";
        std::ifstream weightsFile(weightsPath, std::ios::binary);
        std::string originalWeights((std::istreambuf_iterator<char>(weightsFile)),
                                    std::istreambuf_iterator<char>());
        weightsFile.close();
        
        std::string manifestPath = root + "/r8/manifest.json";
        std::ifstream manifestFile(manifestPath);
        std::string originalManifest((std::istreambuf_iterator<char>(manifestFile)),
                                     std::istreambuf_iterator<char>());
        manifestFile.close();
        
        std::string activePath = root + "/ACTIVE";
        std::ifstream activeFile(activePath);
        std::string originalActive((std::istreambuf_iterator<char>(activeFile)),
                                   std::istreambuf_iterator<char>());
        activeFile.close();
        
        // Try to write the same version again - should fail
        success = writeSeekerModel(root, "r8", model, 8, true, error);
        REQUIRE(success == false);
        REQUIRE(error.empty() == false);
        
        // Verify files are unchanged
        std::ifstream weightsFile2(weightsPath, std::ios::binary);
        std::string newWeights((std::istreambuf_iterator<char>(weightsFile2)),
                               std::istreambuf_iterator<char>());
        weightsFile2.close();
        
        std::ifstream manifestFile2(manifestPath);
        std::string newManifest((std::istreambuf_iterator<char>(manifestFile2)),
                                std::istreambuf_iterator<char>());
        manifestFile2.close();
        
        REQUIRE(newWeights == originalWeights);
        REQUIRE(newManifest == originalManifest);
    }
    
    // Test 9: Activate false doesn't change existing ACTIVE
    SECTION("ActivateFalseDoesNotChangeActive") {
        model = makeRandomSeekerModel(1);
        
        // Write the model first with activate=true to set up initial state
        bool success = writeSeekerModel(root, "r9", model, 9, true, error);
        REQUIRE(success == true);
        REQUIRE(error.empty() == true);
        
        // Store original ACTIVE content
        std::string activePath = root + "/ACTIVE";
        std::ifstream activeFile(activePath);
        std::string originalActive((std::istreambuf_iterator<char>(activeFile)),
                                   std::istreambuf_iterator<char>());
        activeFile.close();
        
        // Write another version with activate=false
        model = makeRandomSeekerModel(2);
        success = writeSeekerModel(root, "r10", model, 10, false, error);
        REQUIRE(success == true);
        REQUIRE(error.empty() == true);
        
        // Verify original ACTIVE is unchanged
        std::string activePath2 = root + "/ACTIVE";
        std::ifstream activeFile2(activePath2);
        std::string newActive((std::istreambuf_iterator<char>(activeFile2)),
                              std::istreambuf_iterator<char>());
        activeFile2.close();
        
        REQUIRE(newActive == originalActive);
    }
    
    // Clean up
    std::filesystem::remove_all(tempDir);
}