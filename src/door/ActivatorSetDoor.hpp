#pragma once

#include "AnimationEnum.hpp"
#include "DoorEnum.hpp"
#include "LoggedEvent.hpp"
#include "iActivator.hpp"

class ActivatorSetDoor : public iActivator {
public:
    ActivatorSetDoor() = default;
    ActivatorSetDoor(DoorEnum door, AnimationEnum animation, LoggedEvent event = {})
        : door(door), animation(animation), event(event) {}

    DoorEnum door = DOOR_WALL;
    AnimationEnum animation = ANIMATION_COUNT;
    LoggedEvent event = {};

    bool activate(Activation& activation) const override;
};
