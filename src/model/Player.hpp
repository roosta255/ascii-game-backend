#pragma once

#include <string>
#include "CodeEnum.hpp"
#include "HASH_MACRO_DECL.hpp"
#include "Text.hpp"

namespace Json {
    class Value;
}

class Dungeon;
class Inventory;
class JsonParameters;
class Match;
class Character;

struct Player {
    static constexpr int INVENTORY_SIZE = 10;

    TextID account;
    int itemStartIndex = -1;

    void startTurn(Match& match);
    void endTurn(Match& match);

    Inventory getInventory(Dungeon& dungeon) const;

    bool isEmpty() const;
    bool isHuman() const;
};

HASH_MACRO_DECL(Player)
