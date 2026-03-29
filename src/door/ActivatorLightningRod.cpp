#include "ActivatorLightningRod.hpp"
#include "Character.hpp"
#include "Codeset.hpp"
#include "DoorEnum.hpp"
#include "Inventory.hpp"
#include "Keyframe.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Player.hpp"
#include "Room.hpp"

bool ActivatorLightningRod::activate(Activation& activation) const {
    bool result = false;
    activation.request.access([&](RequestContext& req) {
        auto& controller = req.controller;
        auto& codeset = req.codeset;
        auto& subject = activation.character;
        auto& inventory = req.player.inventory;
        auto& room = req.room;

        Cardinal direction;
        if (codeset.addFailure(!activation.direction.copy(direction), CODE_ACTIVATION_DIRECTION_NOT_SPECIFIED)) {
            return;
        }

        if (!req.controller.isCharacterKeyerValidation(subject)) {
            return;
        }

        Wall& sourceWall = room.getWall(direction);

        switch (sourceWall.door) {
            case DOOR_LIGHTNING_ROD_AWAKENED:
                if (controller.takeCharacterAction(subject)) {
                    if (controller.giveInventoryItem(inventory, ITEM_CUBE_AWAKENED, req.time, room.roomId, req.isSkippingAnimations)) {
                        sourceWall.setDoor(DOOR_LIGHTNING_ROD_EMPTY, req.time, req.isSkippingAnimations, room.roomId, ANIMATION_SLIDE);
                        result = true;
                        return;
                    }
                }
                break;
            case DOOR_LIGHTNING_ROD_DORMANT:
                if (controller.takeCharacterAction(subject) && controller.giveInventoryItem(inventory, ITEM_CUBE_DORMANT, req.time, room.roomId, req.isSkippingAnimations)) {
                    sourceWall.setDoor(DOOR_LIGHTNING_ROD_EMPTY, req.time, req.isSkippingAnimations, room.roomId, ANIMATION_SLIDE);
                    result = true;
                    return;
                }
                break;
            case DOOR_LIGHTNING_ROD_EMPTY:
                if (controller.takeCharacterAction(subject)) {
                    if (controller.takeInventoryItem(inventory, ITEM_CUBE_DORMANT, req.time, room.roomId, req.isSkippingAnimations)) {
                        sourceWall.setDoor(DOOR_LIGHTNING_ROD_AWAKENED, req.time, req.isSkippingAnimations, room.roomId, ANIMATION_CRUSH);
                        result = true;
                        return;
                    }
                }
                break;
            default:
                codeset.addError(CODE_LIGHTNING_ROD_ACTIVATION_ON_NON_LIGHTNING_ROD);
        }
    });
    return result;
}
