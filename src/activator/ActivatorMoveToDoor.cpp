#include "ActivatorMoveToDoor.hpp"
#include "Codeset.hpp"
#include "EventFlyweight.hpp"
#include "Match.hpp"
#include "MatchController.hpp"

bool ActivatorMoveToDoor::activate(Activation& activation) const {
    bool result = false;
    activation.request.access([&](RequestContext& req) {
        auto& controller = req.controller;
        auto& codeset = req.codeset;
        auto& match = req.match;
        auto& room = req.room;
        auto& subject = activation.character;

        Cardinal direction;
        if (codeset.addFailure(!activation.direction.copy(direction), CODE_ACTIVATION_DIRECTION_NOT_SPECIFIED)) {
            return;
        }

        int conflictingCharacterId;
        if (codeset.addFailure(controller.isDoorOccupied(room.roomId, subject.location.channel, direction, conflictingCharacterId), CODE_OCCUPIED_TARGET_WALL_CELL)) {
            return;
        }

        Wall& next = room.getWall(direction);
        if (codeset.addFailure(!next.isWalkable(codeset.error), CODE_DOOR_FAILED_WALKABILITY)) {
            return;
        }

        if (codeset.addFailure(!subject.takeMove(codeset.error))) {
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
            const auto start = Keyframe::getLatestMovementEnd(rack, req.time);
            const auto keyframe = Keyframe::buildWalking(start, MatchController::MOVE_ANIMATION_DURATION, room.roomId, oldLocation, newLocation, codeset);
            if(!Keyframe::insertKeyframe(rack, keyframe)) {
                codeset.addLog(CODE_ANIMATION_OVERFLOW_IN_MOVE_CHARACTER_TO_FLOOR);
            }
        }

        result = true;
    });
    return result;
}
