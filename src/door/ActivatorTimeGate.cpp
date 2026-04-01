#include "ActivatorTimeGate.hpp"
#include "Character.hpp"
#include "Codeset.hpp"
#include "DoorEnum.hpp"
#include "Keyframe.hpp"
#include "Location.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Player.hpp"
#include "Room.hpp"

bool ActivatorTimeGate::activate(Activation& activation) const {
    bool result = false;
    activation.request.access([&](RequestContext& req) {
        auto& controller = req.controller;
        auto& room = req.room;
        const auto roomId = room.roomId;
        auto& codeset = req.codeset;
        auto& subject = activation.character;
        auto& inventory = req.player.inventory;

        Wall* sourceWallPtr = nullptr;
        activation.targetDoor().access([&](Wall& w) { sourceWallPtr = &w; });
        if (codeset.addFailure(!sourceWallPtr, CODE_ACTIVATION_TARGET_NOT_SPECIFIED)) return;
        Wall& sourceWall = *sourceWallPtr;

        if (codeset.addFailure(!controller.validateDoorNotOccupied(room.roomId, CHANNEL_CORPOREAL, TIME_GATE_DIRECTION), CODE_OCCUPIED_TARGET_TIME_GATE_CELL)) {
            int occupyingId = -1;
            controller.isDoorOccupied(room.roomId, CHANNEL_CORPOREAL, TIME_GATE_DIRECTION, occupyingId);
            RoleEnum occRole = ROLE_EMPTY;
            CodeEnum charError = CODE_UNKNOWN_ERROR;
            req.match.getCharacter(occupyingId, charError).access([&](Character& c) { occRole = c.role; });
            controller.addRequestLoggedEvent(activation, LoggedEvent{
                EVENT_OCCUPIED_DOORWAY,
                { EventComponentKind::ROLE, (int)occRole },
                {},
                { EventComponentKind::DOOR, (int)room.getWall(TIME_GATE_DIRECTION).door },
                TIME_GATE_DIRECTION.getIndex()
            });
            return;
        }

        int delta = 0;
        switch (room.type) {
            case ROOM_TIME_GATE_TO_FUTURE:
                delta = 1;
                break;
            case ROOM_TIME_GATE_TO_PAST:
                delta = -1;
                break;
            default:
                codeset.addError(CODE_TIME_GATE_ACTIVATED_BUT_ROOM_ISNT_TIME_GATE);
                return;
        }

        bool isSuccess = false;
        switch (sourceWall.door) {
            case DOOR_TIME_GATE_AWAKENED:
                {
                    const bool isDeltaValid = req.match.dungeon.rooms.access(room.getDeltaTime(delta), [&](Room& room2) {
                        sourceWall.setDoor(DOOR_TIME_GATE_DORMANT, req.time, req.isSkippingAnimations, roomId, ANIMATION_CRUSH);
                        auto& wall2 = room2.getWall(TIME_GATE_DIRECTION);
                        wall2.setDoor(DOOR_TIME_GATE_DORMANT, req.time, req.isSkippingAnimations, room2.roomId, ANIMATION_CRUSH);

                        const auto newLocation = Location::makeDoor(room2.roomId, subject.location.channel, TIME_GATE_DIRECTION);
                        Location oldLocation;
                        controller.updateCharacterLocation(subject, newLocation, oldLocation);

                        if (controller.takeCharacterMove(subject)) {
                            if (!req.isSkippingAnimations) {
                                const Keyframe keyframe = Keyframe::buildWalking(req.time, MatchController::MOVE_ANIMATION_DURATION, room.roomId, oldLocation, newLocation, codeset);
                                if(!Keyframe::insertKeyframe(Rack<Keyframe>::buildFromArray<Character::MAX_KEYFRAMES>(subject.keyframes), keyframe)) {
                                    codeset.addLog(CODE_ANIMATION_OVERFLOW_IN_ACTIVATE_TIME_GATE);
                                }
                            }
                            isSuccess = true;
                            return;
                        }
                        controller.addRequestLoggedEvent(activation, LoggedEvent{
                            EVENT_NO_MOVES,
                            { EventComponentKind::ROLE, (int)subject.role },
                            {}, {}, -1
                        });
                    });

                    if (codeset.addFailure(!isDeltaValid, CODE_TIME_GATE_ACTIVATED_BUT_NEIGHBOR_DNE)) {
                        return;
                    }
                }
                result = isSuccess;
                return;
            default:
                codeset.addError(CODE_TIME_GATE_ACTIVATED_BUT_WALL_MISSING_AWAKENED_CUBE);
                return;
        }

        codeset.addUnreachableError(__LINE__);
    });
    return result;
}
