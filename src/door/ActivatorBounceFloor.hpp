#pragma once

#include "iActivator.hpp"

class ActivatorBounceFloor : public iActivator {
public:
    ActivatorBounceFloor() = default;
    explicit ActivatorBounceFloor(bool isActorSource) : isActorSource(isActorSource) {}

    // true  = keyframe on Activation.character, targets Activation.target's floor position
    // false = keyframe on Activation.target,    targets Activation.character's floor position
    bool isActorSource = true;

    bool activate(Activation& activation) const override;
};
