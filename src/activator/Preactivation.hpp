#pragma once

#include "Cardinal.hpp"
#include "Maybe.hpp"
#include "Timestamp.hpp"

struct Preactivation {
    std::string playerId;
    int characterId = 0;
    int roomId = 0;
    Maybe<int> targetCharacterId;
    Maybe<int> targetItemIndex;
    Maybe<Cardinal> direction;
    Maybe<int> floorId;
    bool isSkippingAnimations = false;
    bool isSortingState = false;
    Timestamp time = Timestamp();
};
