#pragma once

#include "AnimationEnum.hpp"
#include "TraitBits.hpp"
#include "iActivator.hpp"

// Walks every wall in the dungeon. For each wall whose doorAttributes contain
// all bits of `match`, applies a trait delta to find the new door type:
//   newTraits = (currentTraits | set) - clear
// then transitions the wall to that door if a matching DoorEnum exists.
// Walls whose traits are already correct (no change) are skipped.
class ActivatorSetAllDoorsByTrait : public iActivator {
public:
    ActivatorSetAllDoorsByTrait() = default;
    ActivatorSetAllDoorsByTrait(TraitBits match, TraitBits set, TraitBits clear, AnimationEnum animation)
        : match(match), set(set), clear(clear), animation(animation) {}

    TraitBits match = {};
    TraitBits set = {};
    TraitBits clear = {};
    AnimationEnum animation = ANIMATION_COUNT;

    bool activate(Activation& activation) const override;
};
