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

        if (!controller.isCharacterKeyerValidation(subject)) {
            controller.addRequestLoggedEvent(activation, LoggedEvent{
                EVENT_NOT_KEYER,
                { EventComponentKind::ROLE, (int)subject.role },
                {}, {}, -1
            });
            return;
        }

        const int roomId = req.room.roomId;

        codeset.addFailure(!activation.targetLock().access([&](Wall& sourceWall) {

        switch (sourceWall.door) {
            case DOOR_TIME_GATE_AWAKENED:
                if (!controller.takeCharacterAction(subject)) {
                    controller.addRequestLoggedEvent(activation, LoggedEvent{
                        EVENT_NO_ACTIONS,
                        { EventComponentKind::ROLE, (int)subject.role },
                        {}, {}, -1
                    });
                    break;
                }
                if (!controller.giveInventoryItem(inventory, ITEM_CUBE_AWAKENED, req.time, roomId, req.isSkippingAnimations)) {
                    controller.addRequestLoggedEvent(activation, LoggedEvent{
                        EVENT_INVENTORY_FULL,
                        { EventComponentKind::ROLE, (int)subject.role },
                        {},
                        { EventComponentKind::ITEM, (int)ITEM_CUBE_AWAKENED },
                        -1
                    });
                    break;
                }
                sourceWall.setDoor(DOOR_TIME_GATE_EMPTY, req.time, req.isSkippingAnimations, roomId, ANIMATION_SLIDE);
                result = true;
                return;
            case DOOR_TIME_GATE_DORMANT:
                if (!controller.takeCharacterAction(subject)) {
                    controller.addRequestLoggedEvent(activation, LoggedEvent{
                        EVENT_NO_ACTIONS,
                        { EventComponentKind::ROLE, (int)subject.role },
                        {}, {}, -1
                    });
                    break;
                }
                if (!controller.giveInventoryItem(inventory, ITEM_CUBE_DORMANT, req.time, roomId, req.isSkippingAnimations)) {
                    controller.addRequestLoggedEvent(activation, LoggedEvent{
                        EVENT_INVENTORY_FULL,
                        { EventComponentKind::ROLE, (int)subject.role },
                        {},
                        { EventComponentKind::ITEM, (int)ITEM_CUBE_DORMANT },
                        -1
                    });
                    break;
                }
                sourceWall.setDoor(DOOR_TIME_GATE_EMPTY, req.time, req.isSkippingAnimations, roomId, ANIMATION_SLIDE);
                result = true;
                return;
            case DOOR_TIME_GATE_EMPTY:
                if (!controller.takeCharacterAction(subject)) {
                    controller.addRequestLoggedEvent(activation, LoggedEvent{
                        EVENT_NO_ACTIONS,
                        { EventComponentKind::ROLE, (int)subject.role },
                        {}, {}, -1
                    });
                    break;
                }
                if (!controller.takeInventoryItem(inventory, ITEM_CUBE_AWAKENED, req.time, roomId, req.isSkippingAnimations)) {
                    controller.addRequestLoggedEvent(activation, LoggedEvent{
                        EVENT_MISSING_ITEM,
                        { EventComponentKind::ROLE, (int)subject.role },
                        {},
                        { EventComponentKind::ITEM, (int)ITEM_CUBE_AWAKENED },
                        -1
                    });
                    break;
                }
                sourceWall.setDoor(DOOR_TIME_GATE_AWAKENED, req.time, req.isSkippingAnimations, roomId, ANIMATION_CRUSH);
                result = true;
                return;
            default:
                codeset.addError(CODE_TIME_GATE_ACTIVATION_ON_NON_TIME_GATE);
                return;
        }
        }), CODE_ACTIVATION_TARGET_NOT_SPECIFIED);
    });
    return result;
}
