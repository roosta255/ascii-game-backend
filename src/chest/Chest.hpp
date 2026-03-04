#pragma once

#include <functional>
#include "Character.hpp"
#include "Inventory.hpp"
#include "LockEnum.hpp"

class Dungeon;

struct Chest
{
    static constexpr auto DUNGEON_CHEST_COUNT = 8;
    Inventory inventory;
    LockEnum lock = LOCK_NIL;
    int containerCharacterId = -1;
    int critterCharacterId = -1;

    Pointer<Character> getContainerCharacter(Dungeon& dungeon);
    Pointer<Character> getCritterCharacter(Dungeon& dungeon);
};

std::ostream& operator<<(std::ostream& os, const Chest& rhs);

HASH_MACRO_DECL(Chest)
