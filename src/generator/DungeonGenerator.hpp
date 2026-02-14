#pragma once

#include "Bitstick.hpp"
#include "Maybe.hpp"
#include "Match.hpp"
#include "RemodelEnum.hpp"
#include <string>

class Codeset;

struct DungeonGenerator {
// members
std::string playerId;
int runCharacterId = -1, bossRoomId = -1;
Codeset& codeset;

public:

// members
Maybe<Bitstick<REMODEL_COUNT> > allowlist, blocklist;

// constructors
    DungeonGenerator(const std::string& playerId, const int& runCharacterId, const int& bossRoomId, Codeset& codeset);

// functions
    Maybe<Match> remodel(const Match& source);
    Maybe<Match> remodelLoop(const Match& source, const int loops = 1);
};
