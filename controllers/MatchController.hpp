#pragma once

#include "iMatchStore.hpp"
#include "Match.hpp"
#include "Array.hpp"
#include <string>

class MatchController {
public:
    MatchController(iMatchStore& store);

    bool saveMatch(const Match& output);

    bool loadMatch(const std::string& matchId, Match& output);

private:
    iMatchStore& store_;
};
