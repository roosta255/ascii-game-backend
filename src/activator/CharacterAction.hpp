#pragma once

#include "ActionEnum.hpp"
#include "Cardinal.hpp"
#include "Maybe.hpp"
#include <ostream>

struct CharacterAction {
    ActionEnum type = ACTION_NIL;
    int characterId = 0;
    int roomId = 0;
    Maybe<int> targetCharacterId;
    Maybe<int> targetItemIndex;
    Maybe<Cardinal> direction;
    Maybe<int> floorId;

    // operators
    inline bool operator==(const CharacterAction& rhs)const{
        return type == rhs.type
            && characterId == rhs.characterId
            && roomId == rhs.roomId
            && targetCharacterId == rhs.targetCharacterId
            && targetItemIndex == rhs.targetItemIndex
            && direction == rhs.direction
            && floorId == rhs.floorId;
    }

};

std::ostream& operator<<(std::ostream& os, const CharacterAction& action);
