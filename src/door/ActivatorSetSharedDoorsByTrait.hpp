#pragma once

#include "AnimationEnum.hpp"
#include "LoggedEvent.hpp"
#include "TraitBits.hpp"
#include "iActivator.hpp"

// Transitions the target wall (and its shared neighbor) to the door type determined
// by applying a trait delta to the current wall's doorAttributes:
//   newTraits = (currentTraits | set) - clear
// then looking up the DoorEnum whose doorAttributes exactly match newTraits.
// The neighbor wall receives the same trait delta applied to its own doorAttributes.
class ActivatorSetSharedDoorsByTrait : public iActivator {
public:
    ActivatorSetSharedDoorsByTrait() = default;
    ActivatorSetSharedDoorsByTrait(TraitBits set, TraitBits clear, AnimationEnum animation, LoggedEvent event = {})
        : set(set), clear(clear), animation(animation), event(event) {}

    TraitBits set = {};
    TraitBits clear = {};
    AnimationEnum animation = ANIMATION_COUNT;
    LoggedEvent event = {};

    bool activate(Activation& activation) const override;
};
