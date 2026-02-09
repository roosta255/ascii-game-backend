#pragma once

#include <string>
#include "CodeEnum.hpp"
#include "HASH_MACRO_DECL.hpp"
#include "Inventory.hpp"
#include "Text.hpp"

namespace Json {
    class Value;
}

class JsonParameters;
class Match;
class Character;

struct Player {
    TextID account;
    Inventory inventory;

    void startTurn(Match& match);  // Start turn for all owned characters
    void endTurn(Match& match);    // End turn for all owned characters

    bool isEmpty() const;
    bool isHuman() const;
};

HASH_MACRO_DECL(Player)
