#pragma once

#include "Array.hpp"
#include "PathfindingCounter.hpp"
#include <functional>

class Codeset;
class iLayout;
class Match;

struct Pathfinder {
// constants
    static constexpr auto MAX_PATHFINDING_LOOPS = 2048;
    static constexpr auto MAX_CACHED = 256;

// members
    enum PathfindEnum1 {
        PATHFIND_1_SOLUTION,
        PATHFIND_1_FRONTIER,
        PATHFIND_1_COUNT
    };

    Array<PathfindingCounter, PATHFIND_1_COUNT> counters;
    bool isSolved = 0;

// builders
    static Pathfinder build(const std::string& playerId, int characterId, const int bossRoomId, const Match& match, Codeset& codeset);

// functions
    const PathfindingCounter& getCounter() const;
};
