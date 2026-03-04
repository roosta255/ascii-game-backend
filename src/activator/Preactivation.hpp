#pragma once

#include "CharacterAction.hpp"
#include "Timestamp.hpp"

struct Preactivation {
    CharacterAction action;
    std::string playerId;
    Maybe<int> sourceItemIndex;
    Maybe<int> sourceInventoryId;
    bool isSkippingAnimations = false;
    bool isSortingState = false;
    Timestamp time = Timestamp();
};
