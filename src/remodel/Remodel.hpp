#pragma once

#include "Bitstick.hpp"
#include <functional>
#include "Maybe.hpp"
#include "Pathfinder.hpp"
#include "PathfindingCounter.hpp"
#include "RemodelEnum.hpp"
#include <string>

class Match;
class Codeset;
class Pathfinder;

struct Remodel {
    std::string playerId;
    int characterId, bossRoomId;
    int seed, twist;
    Codeset& codeset;
    Maybe<Bitstick<REMODEL_COUNT> > allowlist, blocklist;
    std::function<bool(const Match&, const PathfindingCounter&)> acceptance;
};
