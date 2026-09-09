#include "ActivatorMoveToFloor.hpp"
#include "BehaviorEventEnum.hpp"
#include "Cardinal.hpp"
#include "Codeset.hpp"
#include "Match.hpp"
#include "MatchController.hpp"

bool ActivatorMoveToFloor::activate(ActivationContext& activation) const {
    bool result = false;
    activation.request.access([&](RequestContext& req) {
        auto& controller = req.controller;
        auto& codeset = req.codeset;
        auto& room = activation.room;
        auto& subject = activation.character;

        int floorId;
        if (codeset.addFailure(!req.floorId.copy(floorId), CODE_ACTIVATION_FLOOR_ID_NOT_SPECIFIED)) {
            return;
        }

        int conflictingCharacterId;
        if (codeset.addFailure(controller.isFloorOccupied(room.roomId, subject.location.channel, floorId, conflictingCharacterId), CODE_OCCUPIED_TARGET_FLOOR_CELL)) {
            RoleEnum occRole = ROLE_EMPTY;
            CodeEnum charError = CODE_UNKNOWN_ERROR;
            req.match.getCharacter(conflictingCharacterId, charError).access([&](Character& c) { occRole = c.role; });
            controller.addRequestLoggedEvent(activation, LoggedEvent{
                EVENT_OCCUPIED_FLOOR,
                { EventComponentKind::ROLE, (int)occRole },
                {}, {}, -1
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

        const auto newLocation = Location::makeFloor(room.roomId, subject.location.channel, floorId);

        Location oldLocation;
        codeset.addFailure(!controller.updateCharacterLocation(subject, newLocation, oldLocation), CODE_UPDATE_CHARACTER_LOCATION_FAILED);

        if (!req.isSkippingAnimations) {
            auto rack = Rack<Keyframe>::buildFromArray<Character::MAX_KEYFRAMES>(subject.keyframes);

            // TriggerEffectTraverseDoor jumps an NPC straight into an adjacent room with a
            // single ACTION_MOVE_TO_FLOOR, skipping the door position a player's
            // floor->door->floor pair would normally pass through. When that happens,
            // oldLocation is still LOCATION_FLOOR but belongs to a different room than
            // room0/newLocation, so Keyframe::buildWalking's Location-overload can't tell
            // this apart from an ordinary same-room floor move and emits a meaningless (or
            // degenerate) WALKING_FROM_FLOOR_TO_FLOOR keyframe. Detect the room change and
            // recover the crossing direction from room0's own wall that leads back to
            // oldLocation's room, rendering the move as a door -> floor walk instead.
            Maybe<Cardinal> crossedThrough;
            if (oldLocation.roomId != room.roomId) {
                for (const Cardinal dir : Cardinal::getAllCardinals()) {
                    if (room.getWall(dir).adjacent == oldLocation.roomId) {
                        crossedThrough = Maybe<Cardinal>(dir);
                        break;
                    }
                }
            }

            const auto keyframe = crossedThrough.isPresent()
                ? Keyframe::buildWalking(req.time, MatchController::MOVE_ANIMATION_DURATION, room.roomId, crossedThrough.orElse(Cardinal::north()), floorId)
                : Keyframe::buildWalking(req.time, MatchController::MOVE_ANIMATION_DURATION, room.roomId, oldLocation, newLocation, codeset);
            if(!Keyframe::insertKeyframe(rack, keyframe)) {
                codeset.addLog(CODE_ANIMATION_OVERFLOW_IN_MOVE_CHARACTER_TO_FLOOR);
            }
        }

        controller.pushTrigger(nullptr, subject.characterId, -1, BEHAVIOR_EVENT_MOVE);
        result = true;
    });
    return result;
}
