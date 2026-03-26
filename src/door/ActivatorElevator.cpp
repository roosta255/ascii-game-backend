#include "Array.hpp"
#include "ActivatorElevator.hpp"
#include "Cardinal.hpp"
#include "Character.hpp"
#include "Codeset.hpp"
#include "DoorEnum.hpp"
#include "DungeonMutator.hpp"
#include "EventFlyweight.hpp"
#include "Inventory.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Player.hpp"
#include "Room.hpp"
#include "Wall.hpp"

bool ActivatorElevator::activate(Activation& activation) const {
    auto& controller = activation.controller;
    auto& codeset = activation.codeset;
    auto& dungeon = activation.match.dungeon;
    auto& match = activation.match;
    auto mutator = DungeonMutator(activation.controller);
    mutator.isElevatorOverride = true;
    auto& subject = activation.character;
    auto& inventory = activation.player.inventory;
    auto& room = activation.room;

    Cardinal outDirection;
    if (codeset.addFailure(!activation.direction.copy(outDirection), CODE_ACTIVATION_DIRECTION_NOT_SPECIFIED)) {
        return false;
    }
    const Cardinal direction = outDirection;

    // Check if character can use keys
    if (!controller.isCharacterKeyerValidation(subject)) {
        return false;
    }

    auto& sourceWall = room.getWall(direction);
    const auto startingAdjacentRoomIds = room.walls.transform([&](const Wall& wall){
        return dungeon.rooms.containsIndex(wall.adjacent) ? Maybe<int>(wall.adjacent) : Maybe<int>::empty();
    });
    const auto getAccessedRoomIdsFromAdjacentRoomIds = [&](const Array<Maybe<int>, 4>& inputAdjacentRoomIds, bool isHighOrElseLow){
        return inputAdjacentRoomIds.transform([&](const Maybe<int>& adjacentRoomIdMaybe){
            return adjacentRoomIdMaybe.map<int>([&](const int& adjacentRoomId){
                return dungeon.rooms.mapIndexAlaConst<int>(adjacentRoomId, [&](const Room& adjacentRoom){
                    return isHighOrElseLow ? adjacentRoom.above : adjacentRoom.below;
                });
            });
        });
    };

    const auto aboveAdjacentRoomIds = getAccessedRoomIdsFromAdjacentRoomIds(startingAdjacentRoomIds, true);
    const auto belowAdjacentRoomIds = getAccessedRoomIdsFromAdjacentRoomIds(startingAdjacentRoomIds, false);
    // this button was clicked to do something
    // this has to figure out what was done, then the next fsm state
    // internal -> could be ascend/descend or pay-higher/pay-lower
    // external -> could be to call the elevator
    // ascend/descend vs pay-higher/pay-lower depends on 1: which direction was activated so that whether the next level is paid for can be determined
    // isInternal vs isExternal
    const auto getKeyStatusOfLevel = [&](const Array<Maybe<int>, 4>& inputAdjacentRoomIds){
        const auto defau1t = Maybe<bool>::empty();
        return inputAdjacentRoomIds.transform([&](const int directionIndex, const Maybe<int>& accessedRoomId){
            return dungeon.rooms.mapIndexAlaConst<Maybe<bool> >(accessedRoomId.orElse(-1), [&](const Room& accessedRoom){
                const DoorEnum accessedDoor = accessedRoom.getWall(Cardinal(directionIndex).getFlip()).door;
                return isKeyed(accessedDoor) ? Maybe<bool>(true) : isKeyless(accessedDoor) ? Maybe<bool>(false) : Maybe<bool>::empty();
            }).orElse(Maybe<bool>::empty());
        }).findFirst(defau1t, [&](const Maybe<bool>& keyStatus){return keyStatus.isPresent();});
    };

    const auto keyStatusAbove = getKeyStatusOfLevel(aboveAdjacentRoomIds);
    const auto keyStatusBelow = getKeyStatusOfLevel(belowAdjacentRoomIds);

    const bool isInternal = room.type == ROOM_ELEVATOR;
    const bool isDirectionHigh = direction == Cardinal::north();
    codeset.addTable(CODE_ELEVATOR_IS_INTERNAL, isInternal);
    if (isInternal) {

        if (codeset.addFailure(!isDirectionHigh && direction != Cardinal::south(), CODE_ELEVATOR_INTERNAL_ISNT_ABOVE_OR_BELOW_DIRECTION)) {
            return false;
        }

        const auto keyStatus = isDirectionHigh ? keyStatusAbove : keyStatusBelow;
        if (codeset.addFailure(keyStatus.isEmpty(), CODE_ELEVATOR_CANNOT_DETERMINE_KEY_STATUS)) {
            // this button state requires another level to the elevator, but the next level's keyed/keyless status could not be determined
            return false;
        }

        const auto isMoving = keyStatus.orElse(false);
        const auto isPaying = !isMoving;

        codeset.addTable(CODE_ELEVATOR_IS_MOVING, isMoving);
        codeset.addTable(CODE_ELEVATOR_IS_PAYING, isPaying);

        if (codeset.addFailure(!controller.takeCharacterAction(subject), CODE_ELEVATOR_FAILED_TO_PAY_ACTION)) {
            return false;
        }

        if (isPaying && codeset.addFailure(!controller.takeInventoryItem(inventory, ITEM_KEY_ELEVATOR, activation.time, room.roomId, activation.isSkippingAnimations), CODE_ELEVATOR_FAILED_TO_PAY_KEY)) {
            return false;
        }

        if (isMoving) {
            // check all door occupancies before moving.
            for (const auto dir: Cardinal::getAllCardinals()) {
                int blockingCharacterId = -1;
                const auto isDoorOccupied = controller.isDoorOccupied(room.roomId, CHANNEL_CORPOREAL, dir, blockingCharacterId);
                if (codeset.addFailure(isDoorOccupied, CODE_ELEVATOR_DOOR_IS_OCCUPIED_BY_CHARACTER)){
                    return false;
                }
            }
        }

        // this will be on the next level, whether directly above or below
        // only moving will update the current level (to close it)
        const auto directUpdateElevatorProperties = DungeonMutator::ElevatorProperties
            { .connectedRoomIds = isDirectionHigh ? aboveAdjacentRoomIds : belowAdjacentRoomIds,
              .isPaid = true // always true whether paying or was already paid
            };

        const auto selfUpdateElevatorProperties = DungeonMutator::ElevatorProperties
            { .connectedRoomIds = startingAdjacentRoomIds,
              .isPaid = true // always true whether paying or was already paid
            };

        const auto subsequentAdjacentRooms = getAccessedRoomIdsFromAdjacentRoomIds(directUpdateElevatorProperties.connectedRoomIds, isDirectionHigh);
        const auto subsequentKeyStatus = getKeyStatusOfLevel(subsequentAdjacentRooms);
        const auto isSubsequentLevelExisting = subsequentKeyStatus.isPresent();
        const auto isSubsequentLevelPaid = subsequentKeyStatus.orElse(false);

        // always mutate the direct
        if (codeset.addFailure(!mutator.setupElevatorLevel(room.roomId, directUpdateElevatorProperties, isMoving, isDirectionHigh ? isSubsequentLevelExisting : true, isDirectionHigh ? true : isSubsequentLevelExisting, isDirectionHigh ? isSubsequentLevelPaid : true, isDirectionHigh ? true : isSubsequentLevelPaid), CODE_ELEVATOR_FAILED_TO_SETUP_ELEVATOR_DIRECT)) {
            return false;
        }

        // moving mutates the self to close it
        if (isMoving && codeset.addFailure(!mutator.setupElevatorLevel(room.roomId, selfUpdateElevatorProperties, false, keyStatusAbove.isPresent(), keyStatusBelow.isPresent(), false, false), CODE_ELEVATOR_FAILED_TO_SETUP_ELEVATOR_SELF)) {
            return false;
        }

        // paying mutates the self to update the button from paying to moving
        if (isPaying && codeset.addFailure(!mutator.setupElevatorLevel(room.roomId, selfUpdateElevatorProperties, true, keyStatusAbove.isPresent(), keyStatusBelow.isPresent(), isDirectionHigh ? true : keyStatusAbove.orElse(false), isDirectionHigh ? keyStatusBelow.orElse(false) : true), CODE_ELEVATOR_FAILED_TO_SETUP_ELEVATOR_SELF)) {
            return false;
        }

        controller.appendEventLog(activation, LoggedEvent{
            EVENT_ELEVATOR,
            { EventComponentKind::ROLE, (int)subject.role },
            {},
            {},
            (int)direction
        });
        return true;
    } else {
        // call elevator button
    }

    return false;
}
