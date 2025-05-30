#include "MatchStoreFile.hpp"
#include <drogon/drogon.h>
#include <fstream>
#include <filesystem> // For C++17 and above
#include <json/json.h>

MatchStoreFile::MatchStoreFile(const std::string& dirPath)
    : dir_(dirPath) {}

bool MatchStoreFile::load(const std::string& matchId, Match& output) {
    std::ifstream file(dir_ + "/" + matchId + ".json");
    if (!file) {
        LOG_ERROR << "Match " << matchId << " file not found";
        return false;
    }

    Json::Value root;
    file >> root;
    if (!output.fromJson(root)) {
        LOG_ERROR << "Match " << matchId << " not parsed";
        return false;
    }
    return true;
}

bool MatchStoreFile::compareAndSwap(const Match& updated) {
    if (!std::filesystem::exists(dir_)) {
        std::filesystem::create_directories(dir_);
    }

    // version 0 has a different kind of existance check
    Match existing;
    const bool isExisting = load(updated.match, existing);
    const bool isInitial = updated.version == 0;
    if(isInitial) {
        if (isExisting) {
            LOG_ERROR << "Version " << updated.version << " save, but match " << updated.match << " already exists";
            return false;
        }
    } else {
        if (!isExisting) {
            LOG_ERROR << "Version " << updated.version << " save, but match " << updated.match <<  "doesnt exist";
            return false;
        }
    }

    if (existing.version != updated.version) {
        LOG_ERROR << "Version " << updated.version << " save, but match " << updated.match << " doesnt match version " << existing.version;
        return false;
    }

    std::ofstream file(dir_ + "/" + updated.match + ".json");
    if (!file) {
        return false;
    }

    Json::Value root;
    Match copy = updated;
    copy.version = updated.version + 1;
    copy.toJson(root);
    file << root;
    return true;
}
