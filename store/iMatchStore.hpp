#pragma once

#include "Match.hpp"
#include <string>

class iMatchStore {
public:
    virtual ~iMatchStore() = default;
    virtual bool load(const std::string& matchId, Match& output) = 0;
    virtual bool compareAndSwap(const Match& updated) = 0;
};
