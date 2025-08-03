#include "JsonParameters.hpp"
#include "FileStore.hpp"
#include <fstream>
#include <filesystem> // For C++17 and above
#include <json/json.h>
#include "make_filename.hpp"
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>

FileStore::FileStore(const std::string& dirPath): dir(dirPath) {}

bool FileStore::version
( const std::string& filename, CodeEnum& error, int& version )
{
    if (!setupDirectory(error)) {
        return false;
    }

    std::ifstream file(getFilePath(filename));
    if (!file) {
        error = CODE_FILE_NOT_FOUND_ON_DISK;
        return false;
    }

    Json::Value root;

    file >> root;

    if (!root.isMember("version")) {
        return false;
    }
    version = root["version"].asUInt();

    return true;
}

bool FileStore::list
( const int limit, const int offset, int& total, CodeEnum& error, std::function<void(const std::string&)> consumer )
{
    if (!setupDirectory(error)) {
        return false;
    }
    std::vector<std::string> matchIds;

    // Collect .json files
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            matchIds.push_back(entry.path().stem().string());  // Removes ".json"
        }
    }

    // Output total metadata.
    total = static_cast<int>(matchIds.size());

    // Sort alphabetically (you could customize to sort by timestamp if you add metadata)
    std::sort(matchIds.begin(), matchIds.end());

    Json::Value result(Json::arrayValue);
    for (int i = offset; i < std::min(offset + limit, static_cast<int>(matchIds.size())); ++i) {
        consumer(matchIds[i]);
    }

    return true;
}

std::string FileStore::getFilePath(const std::string& filename) {
    return dir + "/" + filename + ".json";
}

bool FileStore::setupDirectory(CodeEnum& error) {
    if (!std::filesystem::exists(dir)) {
        std::filesystem::create_directories(dir);
    }
    return true;
}

bool FileStore::load
( const std::string& filename, CodeEnum& error, nlohmann::json& root )
{
    if (!setupDirectory(error)) {
        return false;
    }

    std::ifstream file(getFilePath(filename));
    if (!file) {
        error = CODE_FILE_NOT_FOUND_ON_DISK;
        return false;
    }

    try {
        root = nlohmann::json::parse(file, nullptr, true, true);
    } catch (const std::exception& e) {
        error = CODE_JSON_PARSE_ERROR;
        return false;
    }

    return true;
}

bool FileStore::save
( const bool isInitial, CodeEnum& error, const nlohmann::json& body )
{
    if (!setupDirectory(error)) {
        return false;
    }

    CodeEnum loadError = CODE_SUCCESS;
    nlohmann::json stored;
    nlohmann::json saving = body;
    auto filename = saving["filename"].get<std::string>();

    if (isInitial) {
        saving["filename"] = filename;
    }

    const bool isExisting = load(filename, loadError, stored);
    if (!isExisting && loadError == CODE_FILE_NOT_FOUND_ON_DISK) {
        loadError = CODE_FILE_NOT_FOUND_ON_DISK_FOR_UPDATE;
    }

    const auto versionStored = stored.is_null() ? 0: stored.value("version", 0);
    const auto versionUpdated = saving.value("version", 0);

    if (isInitial && versionUpdated != 0) {
        error = CODE_INITIAL_STORE_BUT_WITH_NONZERO_VERSION;
        return false;
    }

    if (isInitial && isExisting) {
        error = CODE_INITIAL_STORE_BUT_WITH_EXISTING_STORED;
        return false;
    }

    if (!isInitial && !isExisting) {
        error = CODE_SUBSEQUENT_STORE_BUT_WITHOUT_EXISTING_STORED;
        return false;
    }

    if (!isInitial && versionStored != versionUpdated) {
        error = CODE_UPDATE_BUT_WITH_CONFLICTING_VERSION;
        return false;
    }

    if (!isInitial) {
        saving["version"] = versionUpdated + 1;
    }

    std::ofstream file(getFilePath(filename));
    if (!file) {
        error = CODE_FILE_WRITE_FAILED;
        return false;
    }

    try {
        file << saving.dump(4); // Pretty-print with 4-space indent for readability
    } catch (const std::exception& e) {
        error = CODE_JSON_WRITE_FAILED;
        return false;
    }

    return true;
}

