#pragma once

#include "AnimationEnum.hpp"
#include "DoorEnum.hpp"
#include "LoggedEvent.hpp"
#include "iActivator.hpp"

class ActivatorSetSharedDoors : public iActivator {
public:
    ActivatorSetSharedDoors() = default;
    ActivatorSetSharedDoors(DoorEnum sourceDoor, DoorEnum neighborDoor, AnimationEnum animation, LoggedEvent event = {})
        : sourceDoor(sourceDoor), neighborDoor(neighborDoor), animation(animation), event(event) {}

    DoorEnum sourceDoor = DOOR_WALL;
    DoorEnum neighborDoor = DOOR_WALL;
    AnimationEnum animation = ANIMATION_COUNT;
    LoggedEvent event = {};

    bool activate(Activation& activation) const override;
};
