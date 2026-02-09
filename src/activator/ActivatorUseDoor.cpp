#include "ActivatorUseDoor.hpp"
#include "Codeset.hpp"
#include "DoorFlyweight.hpp"
#include "Match.hpp"
#include "MatchController.hpp"

bool ActivatorUseDoor::activate(Activation& activation) const {
    auto& controller = activation.controller;
    auto& codeset = activation.codeset;
    auto& match = activation.match;
    auto& player = activation.player;
    auto& inventory = player.inventory;
    auto& room = activation.room;
    auto& subject = activation.character;

    Cardinal direction;
    if (codeset.addFailure(!activation.direction.copy(direction), CODE_ACTIVATION_DIRECTION_NOT_SPECIFIED)) {
        return false;
    }

    // Get and validate the door
    Wall& wall = room.getWall(direction);

    bool isSuccess = false;
    codeset.addFailure(!wall.accessDoor(codeset.error, [&](const DoorFlyweight& door) {
        if (door.isDoorActionable) {
            if (!door.doorActivator.accessConst([&](const iActivator& activatorIntf) {
                codeset.addFailure(!(isSuccess = activatorIntf.activate(activation)));
            })) {
                codeset.addError(CODE_DOOR_MISSING_ACTIVATOR);
                codeset.setTable(CODE_ACTIVATION_ROOM_ID, room.roomId);
                codeset.setTable(CODE_ACTIVATION_WALL_TYPE, wall.door);
            }
        } else {
            codeset.addError(CODE_DOOR_ACTIVATOR_NOT_CONFIGURED);
            codeset.setTable(CODE_ACTIVATION_ROOM_ID, room.roomId);
            codeset.setTable(CODE_ACTIVATION_WALL_TYPE, wall.door);
        }
    }));
    return isSuccess;
}