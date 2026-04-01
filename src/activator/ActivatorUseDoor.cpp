#include "ActivatorUseDoor.hpp"
#include "Codeset.hpp"
#include "DoorFlyweight.hpp"
#include "Match.hpp"
#include "MatchController.hpp"

bool ActivatorUseDoor::activate(Activation& activation) const {
    bool isSuccess = false;
    activation.request.access([&](RequestContext& req) {
        auto& controller = req.controller;
        auto& codeset = req.codeset;
        auto& room = req.room;
        auto& subject = activation.character;

        codeset.addFailure(!activation.targetDoor().access([&](Wall& wall) {
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
        }), CODE_ACTIVATION_TARGET_NOT_SPECIFIED);
    });
    return isSuccess;
}
