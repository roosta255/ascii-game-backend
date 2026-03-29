#include "ActivatorTimeGateCube.hpp"
#include "Room.hpp"
#include "Character.hpp"
#include "Codeset.hpp"
#include "Inventory.hpp"
#include "Keyframe.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Player.hpp"
#include "DoorEnum.hpp"

bool ActivatorTimeGateCube::activate(Activation& activation) const {
    bool result = false;
    activation.request.access([&](RequestContext& req) {
        MatchController& controller = req.controller;
        Codeset& codeset = req.codeset;
        Character& subject = activation.character;
        auto& inventory = req.player.inventory;

        Cardinal direction;
        if (codeset.addFailure(!activation.direction.copy(direction), CODE_ACTIVATION_DIRECTION_NOT_SPECIFIED)) {
            return;
        }

        if (!controller.isCharacterKeyerValidation(subject)) {
            return;
        }

        Wall& sourceWall = req.room.getWall(direction);
        const int roomId = req.room.roomId;

        switch (sourceWall.door) {
            case DOOR_TIME_GATE_AWAKENED:
                if (controller.takeCharacterAction(subject) && controller.giveInventoryItem(inventory, ITEM_CUBE_AWAKENED, req.time, roomId, req.isSkippingAnimations)) {
                    sourceWall.setDoor(DOOR_TIME_GATE_EMPTY, req.time, req.isSkippingAnimations, roomId, ANIMATION_SLIDE);
                    result = true;
                    return;
                }
                break;
            case DOOR_TIME_GATE_DORMANT:
                if (controller.takeCharacterAction(subject) && controller.giveInventoryItem(inventory, ITEM_CUBE_DORMANT, req.time, roomId, req.isSkippingAnimations)) {
                    sourceWall.setDoor(DOOR_TIME_GATE_EMPTY, req.time, req.isSkippingAnimations, roomId, ANIMATION_SLIDE);
                    result = true;
                    return;
                }
                break;
            case DOOR_TIME_GATE_EMPTY:
                if (controller.takeCharacterAction(subject) && controller.takeInventoryItem(inventory, ITEM_CUBE_AWAKENED, req.time, roomId, req.isSkippingAnimations)) {
                    sourceWall.setDoor(DOOR_TIME_GATE_AWAKENED, req.time, req.isSkippingAnimations, roomId, ANIMATION_CRUSH);
                    result = true;
                    return;
                }
                break;
            default:
                codeset.addError(CODE_TIME_GATE_ACTIVATION_ON_NON_TIME_GATE);
                return;
        }
    });
    return result;
}
