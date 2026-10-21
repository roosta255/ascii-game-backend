#include "model_store.hpp"
#include "../projection/audience_views.hpp"
#include "../seeker/feature_schema.hpp"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <iostream>

namespace hider_seeker {

bool writeSeekerModel(const std::string& root, const std::string& version,
  const SeekerModel& model, uint64_t seed, bool activate,
  std::string& error) {
    // Create the root directory if it doesn't exist
    try {
        std::filesystem::create_directories(root);
    } catch (const std::exception&) {
        error = "Failed to create root directory";
        return false;
    }

    // Construct the version directory path
    std::filesystem::path versionDir = std::filesystem::path(root) / version;

    // Check if the version directory already exists
    if (std::filesystem::exists(versionDir)) {
        error = "Version directory already exists";
        return false;
    }

    // Create the version directory
    try {
        std::filesystem::create_directories(versionDir);
    } catch (const std::exception&) {
        error = "Failed to create version directory";
        return false;
    }

    // Write weights.bin
    std::filesystem::path weightsPath = versionDir / "weights.bin";
    std::string bytes = seekerModelBytes(model);
    
    if (bytes.size() != SEEKER_MODEL_BYTES) {
        error = "Incorrect number of bytes in model";
        return false;
    }
    
    std::ofstream weightsFile(weightsPath, std::ios::binary);
    if (!weightsFile.is_open()) {
        error = "Failed to open weights file for writing";
        return false;
    }
    
    weightsFile.write(bytes.c_str(), bytes.size());
    weightsFile.close();
    
    // Write manifest.json
    std::filesystem::path manifestPath = versionDir / "manifest.json";
    nlohmann::json manifest;
    manifest["format"] = "hider-seeker-seeker-mlp";
    manifest["format_version"] = 1;
    manifest["model_version"] = version;
    manifest["feature_version"] = SEEKER_FEATURE_VERSION;
    manifest["layers"] = {43, 32, 16, 3};
    manifest["fixed_point_shift"] = 8;
    manifest["weights_fnv1a64"] = fnv1a64Hex(bytes);
    manifest["created_by"] = "seeker_model_init";
    manifest["seed"] = seed;
    
    std::ofstream manifestFile(manifestPath);
    if (!manifestFile.is_open()) {
        error = "Failed to open manifest file for writing";
        return false;
    }
    
    manifestFile << manifest.dump(2) << "\n";
    manifestFile.close();
    
    // If activate is true, update the ACTIVE file
    if (activate) {
        std::filesystem::path activePath = std::filesystem::path(root) / "ACTIVE";
        std::ofstream activeFile(activePath);
        if (!activeFile.is_open()) {
            error = "Failed to open ACTIVE file for writing";
            return false;
        }
        activeFile << version << "\n";
        activeFile.close();
    }
    
    error.clear();
    return true;
}

bool loadActiveSeekerModel(const std::string& root,
  SeekerModel& model, std::string& version,
  std::string& error) {
    // Read the ACTIVE file
    std::filesystem::path activePath = std::filesystem::path(root) / "ACTIVE";
    std::ifstream activeFile(activePath);
    if (!activeFile.is_open()) {
        error = "ACTIVE file missing";
        return false;
    }
    
    std::getline(activeFile, version);
    activeFile.close();
    
    // Trim whitespace from version
    size_t start = version.find_first_not_of(" \t\n\r\f\v");
    if (start == std::string::npos) {
        error = "ACTIVE file empty";
        return false;
    }
    
    size_t end = version.find_last_not_of(" \t\n\r\f\v");
    version = version.substr(start, end - start + 1);
    
    // Check if version is empty after trimming
    if (version.empty()) {
        error = "ACTIVE file empty";
        return false;
    }
    
    // Construct the version directory path
    std::filesystem::path versionDir = std::filesystem::path(root) / version;
    
    // Verify that the version directory exists
    if (!std::filesystem::exists(versionDir)) {
        error = "missing";
        return false;
    }
    
    // Verify that manifest.json exists
    std::filesystem::path manifestPath = versionDir / "manifest.json";
    if (!std::filesystem::exists(manifestPath)) {
        error = "missing";
        return false;
    }
    
    // Parse manifest.json
    std::ifstream manifestFile(manifestPath);
    if (!manifestFile.is_open()) {
        error = "manifest";
        return false;
    }
    
    std::string manifestContent((std::istreambuf_iterator<char>(manifestFile)),
                                std::istreambuf_iterator<char>());
    manifestFile.close();
    
    auto j = nlohmann::json::parse(manifestContent, nullptr, false);
    if (j.is_discarded()) {
        error = "manifest";
        return false;
    }
    
    // Validate manifest fields
    if (j.value("format", "") != "hider-seeker-seeker-mlp") {
        error = "format";
        return false;
    }
    
    if (j.value("format_version", 0) != 1) {
        error = "format_version";
        return false;
    }
    
    if (j.value("feature_version", 0) != SEEKER_FEATURE_VERSION) {
        error = "feature_version";
        return false;
    }
    
    auto layers = j.value("layers", nlohmann::json::array());
    if (layers.size() != 4 || 
        layers[0] != 43 || layers[1] != 32 || layers[2] != 16 || layers[3] != 3) {
        error = "layers";
        return false;
    }
    
    // Verify that weights.bin exists
    std::filesystem::path weightsPath = versionDir / "weights.bin";
    if (!std::filesystem::exists(weightsPath)) {
        error = "missing";
        return false;
    }
    
    // Check the size of weights.bin
    auto weightsSize = std::filesystem::file_size(weightsPath);
    if (weightsSize != SEEKER_MODEL_BYTES) {
        error = "size";
        return false;
    }
    
    // Read weights.bin
    std::ifstream weightsFile(weightsPath, std::ios::binary);
    if (!weightsFile.is_open()) {
        error = "Failed to open weights file for reading";
        return false;
    }
    
    std::string weightBytes((std::istreambuf_iterator<char>(weightsFile)),
                            std::istreambuf_iterator<char>());
    weightsFile.close();
    
    // Check the checksum
    std::string computedChecksum = fnv1a64Hex(weightBytes);
    std::string manifestChecksum = j.value("weights_fnv1a64", "");
    if (computedChecksum != manifestChecksum) {
        error = "checksum";
        return false;
    }
    
    // Load the model from bytes
    if (!seekerModelFromBytes(weightBytes, model)) {
        error = "Failed to load model from bytes";
        return false;
    }
    
    error.clear();
    return true;
}

} // namespace hider_seeker