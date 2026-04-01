#include "ActivatorSetSharedDoorsByTrait.hpp"
#include "Cardinal.hpp"
#include "Codeset.hpp"
#include "DoorFlyweight.hpp"
#include "EventFlyweight.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Room.hpp"

bool ActivatorSetSharedDoorsByTrait::activate(Activation& activation) const {
    bool isSuccess = false;
    activation.request.access([&](RequestContext& req) {
        auto& codeset = req.codeset;
        auto& room = req.room;

        Cardinal direction;
        if (codeset.addFailure(!activation.direction.copy(direction), CODE_ACTIVATION_DIRECTION_NOT_SPECIFIED)) {
            return;
        }

        Wall* sourceWallPtr = nullptr;
        activation.targetWall().access([&](Wall& w) { sourceWallPtr = &w; });
        if (codeset.addFailure(!sourceWallPtr, CODE_ACTIVATION_TARGET_NOT_SPECIFIED)) return;
        Wall& sourceWall = *sourceWallPtr;

        DoorEnum sourceDoor = DOOR_WALL;
        DoorEnum neighborDoor = DOOR_WALL;

        // Compute source door from current traits + delta
        DoorFlyweight::getFlyweights().accessConst((int)sourceWall.door, [&](const DoorFlyweight& fw) {
            const TraitBits newTraits = (fw.doorAttributes | set) - clear;
            codeset.addFailure(!DoorFlyweight::findByTraits(newTraits, sourceDoor),
                CODE_SET_SHARED_DOORS_BY_TRAIT_NO_MATCHING_DOOR);
        });
        if (codeset.isAnyFailure) return;

        const bool isNeighborAccessed = req.match.dungeon.accessWallNeighbor(room, direction,
            [&](Wall& neighborWall, Room& neighbor, int neighborId) {
                // Compute neighbor door from its own current traits + delta
                DoorFlyweight::getFlyweights().accessConst((int)neighborWall.door, [&](const DoorFlyweight& fw) {
                    const TraitBits newTraits = (fw.doorAttributes | set) - clear;
                    codeset.addFailure(!DoorFlyweight::findByTraits(newTraits, neighborDoor),
                        CODE_SET_SHARED_DOORS_BY_TRAIT_NO_MATCHING_DOOR);
                });
                if (codeset.isAnyFailure) return;

                const Timestamp doorTime = req.time + MatchController::BOUNCE_LOCK_ANIMATION_DURATION / 2;
                sourceWall.setDoor(sourceDoor, doorTime, req.isSkippingAnimations, room.roomId, animation);
                neighborWall.setDoor(neighborDoor, doorTime, req.isSkippingAnimations, neighborId, animation);
                isSuccess = true;

                if (event.action != EVENT_NIL) {
                    LoggedEvent toLog = event;
                    toLog.actor     = { EventComponentKind::ROLE, (int)activation.character.role };
                    toLog.target    = { EventComponentKind::DOOR, (int)sourceDoor };
                    toLog.direction = (int)direction;
                    req.controller.addLoggedEvent(activation, room.roomId, toLog);
                }
            }
        );

        codeset.addFailure(!isNeighborAccessed, CODE_INACCESSIBLE_NEIGHBORING_WALL);
    });
    return isSuccess;
}
