#pragma once

#include <functional>
#include "Array.hpp"
#include "Character.hpp"
#include "Inventory.hpp"
#include "Keyframe.hpp"
#include "LockEnum.hpp"

class Dungeon;

struct Chest
{
    static constexpr auto DUNGEON_CHEST_COUNT = 8;
    static constexpr int MAX_KEYFRAMES = 2;
    Inventory inventory;
    LockEnum lock = LOCK_NIL;
    int containerCharacterId = -1;
    int critterCharacterId = -1;
    Array<Keyframe, MAX_KEYFRAMES> keyframes;

    Pointer<Character> getContainerCharacter(Dungeon& dungeon);
    Pointer<Character> getCritterCharacter(Dungeon& dungeon);
};

std::ostream& operator<<(std::ostream& os, const Chest& rhs);

HASH_MACRO_DECL(Chest)
