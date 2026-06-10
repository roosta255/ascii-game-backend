#pragma once

#include "CharacterAction.hpp"
#include "TargetPreactivationEntity.hpp"
#include "Timestamp.hpp"

struct Preactivation {
    CharacterAction action;
    std::string playerId;
    TargetPreactivationEntity targetPreactivationEntity;
    Maybe<int> sourceItemIndex;  // absolute index into dungeon.items[]
    bool isSkippingAnimations = false;
    bool isSortingState = false;
    bool isSkippingLogging = false;
    Timestamp time = Timestamp();
};
