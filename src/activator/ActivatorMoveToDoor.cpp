#include "ActivatorMoveToDoor.hpp"
#include "Codeset.hpp"
#include "Match.hpp"
#include "MatchController.hpp"

bool ActivatorMoveToDoor::activate(Activation& activation) const {
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

    // Check for occupied target cell
    int conflictingCharacterId;
    if (codeset.addFailure(controller.isDoorOccupied(room.roomId, subject.location.channel, direction, conflictingCharacterId), CODE_OCCUPIED_TARGET_WALL_CELL)) {
        return false;
    }

    Wall& next = room.getWall(direction);
    if (codeset.addFailure(!next.isWalkable(codeset.error), CODE_DOOR_FAILED_WALKABILITY)) {
        return false;
    }

    // take move, first mutation so no going back
    if (codeset.addFailure(!subject.takeMove(codeset.error))) {
        return false;
    }

    // most doors do share a wall, and it can be assumed that if the next room 1) exists 2) is walkable, then that's a 2-way door.
    bool isShared = false;
    match.dungeon.rooms.access(next.adjacent, [&](Room& room2){
        // hide this error because it's not a problem
        codeset.addFailure(!room2.getWall(direction.getFlip()).readIsSharedDoorway(codeset.error, isShared));
    });

    const auto newLocation = isShared ? Location::makeSharedDoor(next.adjacent, subject.location.channel, direction.getFlip()) : Location::makeDoor(room.roomId, subject.location.channel, direction);

    Location oldLocation;
    controller.updateCharacterLocation(subject, newLocation, oldLocation);

    if (!activation.isSkippingAnimations) {
        const auto keyframe = Keyframe::buildWalking(activation.time, MatchController::MOVE_ANIMATION_DURATION, room.roomId, oldLocation, newLocation, codeset);
        if(!Keyframe::insertKeyframe(Rack<Keyframe>::buildFromArray<Character::MAX_KEYFRAMES>(subject.keyframes), keyframe)) {
            codeset.addLog(CODE_ANIMATION_OVERFLOW_IN_MOVE_CHARACTER_TO_FLOOR);
        }
    }

    return true;
}