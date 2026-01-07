#pragma once

#include <string>

#include "Character.hpp"
#include "Player.hpp"
#include "Inventory.hpp"

class Match;

struct Builder {
    Player player;
    Character character;
    Inventory inventory;

    void startTurn(Match& match);  // Start builder's turn
    void endTurn(Match& match);    // End builder's turn
};