#pragma once

#include <string>

#include "Character.hpp"
#include "HASH_MACRO_DECL.hpp"
#include "Player.hpp"

class Match;

struct Builder {
    Player player;
    Character character;

    void startTurn(Match& match);  // Start builder's turn
    void endTurn(Match& match);    // End builder's turn
};

HASH_MACRO_DECL(Builder)
