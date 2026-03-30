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
            controller.addRequestLoggedEvent(activation, LoggedEvent{
                EVENT_NOT_KEYER,
                { EventComponentKind::ROLE, (int)subject.role },
                {}, {}, -1
            });
            return;
        }

        Wall& sourceWall = room.getWall(direction);

        switch (sourceWall.door) {
            case DOOR_LIGHTNING_ROD_AWAKENED:
                if (!controller.takeCharacterAction(subject)) {
                    controller.addRequestLoggedEvent(activation, LoggedEvent{
                        EVENT_NO_ACTIONS,
                        { EventComponentKind::ROLE, (int)subject.role },
                        {}, {}, -1
                    });
                    break;
                }
                if (!controller.giveInventoryItem(inventory, ITEM_CUBE_AWAKENED, req.time, room.roomId, req.isSkippingAnimations)) {
                    controller.addRequestLoggedEvent(activation, LoggedEvent{
                        EVENT_INVENTORY_FULL,
                        { EventComponentKind::ROLE, (int)subject.role },
                        {},
                        { EventComponentKind::ITEM, (int)ITEM_CUBE_AWAKENED },
                        -1
                    });
                    break;
                }
                sourceWall.setDoor(DOOR_LIGHTNING_ROD_EMPTY, req.time, req.isSkippingAnimations, room.roomId, ANIMATION_SLIDE);
                result = true;
                return;
            case DOOR_LIGHTNING_ROD_DORMANT:
                if (!controller.takeCharacterAction(subject)) {
                    controller.addRequestLoggedEvent(activation, LoggedEvent{
                        EVENT_NO_ACTIONS,
                        { EventComponentKind::ROLE, (int)subject.role },
                        {}, {}, -1
                    });
                    break;
                }
                if (!controller.giveInventoryItem(inventory, ITEM_CUBE_DORMANT, req.time, room.roomId, req.isSkippingAnimations)) {
                    controller.addRequestLoggedEvent(activation, LoggedEvent{
                        EVENT_INVENTORY_FULL,
                        { EventComponentKind::ROLE, (int)subject.role },
                        {},
                        { EventComponentKind::ITEM, (int)ITEM_CUBE_DORMANT },
                        -1
                    });
                    break;
                }
                sourceWall.setDoor(DOOR_LIGHTNING_ROD_EMPTY, req.time, req.isSkippingAnimations, room.roomId, ANIMATION_SLIDE);
                result = true;
                return;
            case DOOR_LIGHTNING_ROD_EMPTY:
                if (!controller.takeCharacterAction(subject)) {
                    controller.addRequestLoggedEvent(activation, LoggedEvent{
                        EVENT_NO_ACTIONS,
                        { EventComponentKind::ROLE, (int)subject.role },
                        {}, {}, -1
                    });
                    break;
                }
                if (!controller.takeInventoryItem(inventory, ITEM_CUBE_DORMANT, req.time, room.roomId, req.isSkippingAnimations)) {
                    controller.addRequestLoggedEvent(activation, LoggedEvent{
                        EVENT_MISSING_ITEM,
                        { EventComponentKind::ROLE, (int)subject.role },
                        {},
                        { EventComponentKind::ITEM, (int)ITEM_CUBE_DORMANT },
                        -1
                    });
                    break;
                }
                sourceWall.setDoor(DOOR_LIGHTNING_ROD_AWAKENED, req.time, req.isSkippingAnimations, room.roomId, ANIMATION_CRUSH);
                result = true;
                return;
            default:
                codeset.addError(CODE_LIGHTNING_ROD_ACTIVATION_ON_NON_LIGHTNING_ROD);
        }
    });
    return result;
}
