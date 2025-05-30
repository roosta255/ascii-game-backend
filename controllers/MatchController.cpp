#include "MatchController.hpp"
#include <sstream>

MatchController::MatchController(iMatchStore& store)
    : store_(store) {}

bool MatchController::saveMatch(const Match& match) {
    return store_.compareAndSwap(match);
}

bool MatchController::loadMatch(const std::string& matchId, Match& output) {
    return store_.load(matchId, output);
}
