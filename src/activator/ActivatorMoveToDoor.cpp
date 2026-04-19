#include "ActivatorMoveToDoor.hpp"
#include "Codeset.hpp"
#include "EventFlyweight.hpp"
#include "Match.hpp"
#include "MatchController.hpp"

bool ActivatorMoveToDoor::activate(ActivationContext& activation) const {
    bool result = false;
    activation.request.access([&](RequestContext& req) {
        auto& controller = req.controller;
        auto& codeset = req.codeset;
        auto& match = req.match;
        auto& room = activation.room;
        auto& subject = activation.character;

        Cardinal direction;
        if (codeset.addFailure(!activation.direction.copy(direction), CODE_ACTIVATION_DIRECTION_NOT_SPECIFIED)) {
            return;
        }

        Wall* nextPtr = nullptr;
        activation.targetDoor().access([&](Wall& w) { nextPtr = &w; });
        if (codeset.addFailure(!nextPtr, CODE_ACTIVATION_TARGET_NOT_SPECIFIED)) return;
        Wall& next = *nextPtr;

        int conflictingCharacterId;
        if (codeset.addFailure(controller.isDoorOccupied(room.roomId, subject.location.channel, direction, conflictingCharacterId), CODE_OCCUPIED_TARGET_WALL_CELL)) {
            RoleEnum occRole = ROLE_EMPTY;
            CodeEnum charError = CODE_UNKNOWN_ERROR;
            match.getCharacter(conflictingCharacterId, charError).access([&](Character& c) { occRole = c.role; });
            controller.addRequestLoggedEvent(activation, LoggedEvent{
                EVENT_OCCUPIED_DOORWAY,
                { EventComponentKind::ROLE, (int)occRole },
                {},
                { EventComponentKind::DOOR, (int)next.door },
                direction.getIndex()
            });
            return;
        }

        if (codeset.addFailure(!next.isWalkable(codeset.error), CODE_DOOR_FAILED_WALKABILITY)) {
            controller.addRequestLoggedEvent(activation, LoggedEvent{
                EVENT_NOT_WALKABLE,
                { EventComponentKind::ROLE, (int)subject.role },
                {},
                { EventComponentKind::DOOR, (int)next.door },
                direction.getIndex()
            });
            return;
        }

        if (codeset.addFailure(!subject.takeMove(codeset.error))) {
            controller.addRequestLoggedEvent(activation, LoggedEvent{
                EVENT_NO_MOVES,
                { EventComponentKind::ROLE, (int)subject.role },
                {}, {}, -1
            });
            return;
        }

        bool isShared = false;
        match.dungeon.rooms.access(next.adjacent, [&](Room& room2){
            codeset.addFailure(!room2.getWall(direction.getFlip()).readIsSharedDoorway(codeset.error, isShared));
        });

        const auto newLocation = isShared ? Location::makeSharedDoor(next.adjacent, subject.location.channel, direction.getFlip()) : Location::makeDoor(room.roomId, subject.location.channel, direction);

        Location oldLocation;
        controller.updateCharacterLocation(subject, newLocation, oldLocation);

        controller.addLoggedEvent(activation, room.roomId, LoggedEvent{
            EVENT_MOVE_TO_DOOR,
            { EventComponentKind::ROLE, (int)subject.role },
            {},
            { EventComponentKind::DOOR, (int)next.door },
            (int)direction
        });

        if (!req.isSkippingAnimations) {
            auto rack = Rack<Keyframe>::buildFromArray<Character::MAX_KEYFRAMES>(subject.keyframes);
            const auto keyframe = Keyframe::buildWalking(req.time, MatchController::MOVE_ANIMATION_DURATION, room.roomId, oldLocation, newLocation, codeset);
            if(!Keyframe::insertKeyframe(rack, keyframe)) {
                codeset.addLog(CODE_ANIMATION_OVERFLOW_IN_MOVE_CHARACTER_TO_FLOOR);
            }
        }

        result = true;
    });
    return result;
}
