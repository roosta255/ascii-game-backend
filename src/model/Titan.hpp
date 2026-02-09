#pragma once

#include <string>

#include "HASH_MACRO_DECL.hpp"
#include "Player.hpp"

namespace Json {
    class Value;
}

class JsonParameters;
class Match;

struct Titan {
    Player player;

    void startTurn(Match& match);  // Start titan's turn
    void endTurn(Match& match);    // End titan's turn
};

HASH_MACRO_DECL(Titan)
