#pragma once

#include "Match.hpp"
#include "iMatchStore.hpp"
#include <string>

class MatchStoreFile : public iMatchStore {
public:
    MatchStoreFile(const std::string& dirPath);
    bool load(const std::string& matchId, Match& output) override;
    bool compareAndSwap(const Match& updated) override;

private:
    std::string dir_;
};
