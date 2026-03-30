#include "ActivatorLadder.hpp"
#include "Codeset.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Player.hpp"
#include "DoorEnum.hpp"

bool ActivatorLadder::activate(Activation& activation) const {
    bool result = false;
    activation.request.access([&](RequestContext& req) {
        auto& controller = req.controller;
        auto& codeset = req.codeset;
        auto& subject = activation.character;
        auto& room = req.room;
        const auto roomId = room.roomId;

        Cardinal direction;
        if (codeset.addFailure(!activation.direction.copy(direction), CODE_ACTIVATION_DIRECTION_NOT_SPECIFIED)) {
            return;
        }

        if (codeset.addFailure(!controller.validateDoorNotOccupied(roomId, CHANNEL_CORPOREAL, direction), CODE_OCCUPIED_TARGET_TIME_GATE_CELL)) {
            int occupyingId = -1;
            controller.isDoorOccupied(roomId, CHANNEL_CORPOREAL, direction, occupyingId);
            RoleEnum occRole = ROLE_EMPTY;
            CodeEnum charError = CODE_UNKNOWN_ERROR;
            req.match.getCharacter(occupyingId, charError).access([&](Character& c) { occRole = c.role; });
            controller.addRequestLoggedEvent(activation, LoggedEvent{
                EVENT_OCCUPIED_DOORWAY,
                { EventComponentKind::ROLE, (int)occRole },
                {},
                { EventComponentKind::DOOR, (int)room.getWall(direction).door },
                direction.getIndex()
            });
            return;
        }

        Location newLocation;
        int neighborRoomId = -1;
        switch (room.getWall(direction).door) {
            case DOOR_LADDER_1_TOP:
                neighborRoomId = room.below;
                newLocation = Location::makeShaftBottom(room.below, subject.location.channel, direction);
                break;
            case DOOR_LADDER_1_BOTTOM:
                neighborRoomId = room.above;
                newLocation = Location::makeShaftTop(room.above, subject.location.channel, direction);
                break;
            default:
                codeset.addError(CODE_ACTIVATOR_LADDER_NEEDS_LADDER_DOOR_TYPE);
                return;
        }

        if (!controller.validateDoorNotOccupied(neighborRoomId, CHANNEL_CORPOREAL, direction)) {
            int occupyingId = -1;
            controller.isDoorOccupied(neighborRoomId, CHANNEL_CORPOREAL, direction, occupyingId);
            RoleEnum occRole = ROLE_EMPTY;
            CodeEnum charError = CODE_UNKNOWN_ERROR;
            req.match.getCharacter(occupyingId, charError).access([&](Character& c) { occRole = c.role; });
            DoorEnum neighborDoor = room.getWall(direction).door;
            req.match.dungeon.rooms.accessConst(neighborRoomId, [&](const Room& nr) { neighborDoor = nr.getWall(direction).door; });
            controller.addRequestLoggedEvent(activation, LoggedEvent{
                EVENT_OCCUPIED_DOORWAY,
                { EventComponentKind::ROLE, (int)occRole },
                {},
                { EventComponentKind::DOOR, (int)neighborDoor },
                direction.getIndex()
            });
            return;
        }

        bool isSuccess = false;
        const bool isNeighborValid = req.match.dungeon.rooms.access(neighborRoomId, [&](Room& room2) {
            Location oldLocation;
            controller.updateCharacterLocation(subject, newLocation, oldLocation);

            if (controller.takeCharacterMove(subject)) {
                if (!req.isSkippingAnimations) {
                    const Keyframe keyframe = Keyframe::buildWalking(req.time, MatchController::MOVE_ANIMATION_DURATION, room.roomId, oldLocation, newLocation, codeset);
                    if(!Keyframe::insertKeyframe(Rack<Keyframe>::buildFromArray<Character::MAX_KEYFRAMES>(subject.keyframes), keyframe)) {
                        codeset.addLog(CODE_ANIMATION_OVERFLOW_IN_ACTIVATE_LADDER);
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

        if (codeset.addFailure(!isNeighborValid, CODE_LADDER_MISSING_NEIGHBORING_ROOM)) {
            codeset.setTable(CODE_MISSING_NEIGHBORING_ROOM, neighborRoomId);
        }

        result = isSuccess;
    });
    return result;
}
