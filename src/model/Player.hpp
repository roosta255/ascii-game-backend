#pragma once

#include <string>
#include "CodeEnum.hpp"
#include "Inventory.hpp"

namespace Json {
    class Value;
}

class JsonParameters;
class Match;
class Character;

struct Player {
    std::string account;
    Inventory inventory;

    void startTurn(Match& match);  // Start turn for all owned characters
    void endTurn(Match& match);    // End turn for all owned characters

    bool isEmpty() const;
    bool isHuman() const;
};
