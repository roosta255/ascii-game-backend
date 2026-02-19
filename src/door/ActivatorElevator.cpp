#include "Array.hpp"
#include "ActivatorElevator.hpp"
#include "Cardinal.hpp"
#include "Character.hpp"
#include "Codeset.hpp"
#include "DoorEnum.hpp"
#include "DungeonMutator.hpp"
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
    const auto adjacentRoomIds = room.walls.transform([&](const Wall& wall){
        return dungeon.rooms.containsIndex(wall.adjacent) ? Maybe<int>(wall.adjacent) : Maybe<int>::empty();
    });
    const auto getAccessedRoomIdsFromAdjacentRoomIds = [&](const Array<Maybe<int>, 4>& startingAdjacentRoomIds, std::function<int(const Room&)> accessor){
        return startingAdjacentRoomIds.transform([&](const Maybe<int>& adjacentRoomIdMaybe){
            return adjacentRoomIdMaybe.map<int>([&](const int& adjacentRoomId){
                return dungeon.rooms.mapIndexAlaConst<int>(adjacentRoomId, [&](const Room& adjacentRoom){
                    return accessor(adjacentRoom);
                });
            });
        });
    };

    // this button was clicked to do something
    // this has to figure out what was done, then the next fsm state
    // internal -> could be ascend/descend or pay-higher/pay-lower
    // external -> could be to call the elevator
    // ascend/descend vs pay-higher/pay-lower depends on 1: which direction was activated so that whether the next level is paid for can be determined
    // isInternal vs isExternal
    const auto getKeyStatusViaAccessor = [&](std::function<int(const Room&)> accessor){
        const auto defau1t = Maybe<bool>::empty();
        return getAccessedRoomIdsFromAdjacentRoomIds(adjacentRoomIds, accessor).transform([&](const int directionIndex, const Maybe<int>& accessedRoomId){
            return dungeon.rooms.mapIndexAlaConst<Maybe<bool> >(accessedRoomId.orElse(-1), [&](const Room& accessedRoom){
                const DoorEnum accessedDoor = DoorEnum(accessedRoom.getWall(Cardinal(directionIndex).getFlip()).door);
                return isKeyed(accessedDoor) ? Maybe<bool>(true) : isKeyless(accessedDoor) ? Maybe<bool>(false) : Maybe<bool>::empty();
            }).orElse(Maybe<bool>::empty());
        }).findFirst(defau1t, [&](const Maybe<bool>& keyStatus){return keyStatus.isPresent();});
    };

    const auto accessorAbove = [&](const Room& room){return room.above;};
    const auto accessorBelow = [&](const Room& room){return room.below;};

    const auto keyStatusAbove = getKeyStatusViaAccessor(accessorAbove);
    const auto keyStatusBelow = getKeyStatusViaAccessor(accessorBelow);

    const bool isInternal = room.type == ROOM_ELEVATOR;
    const bool isDirectionHigh = direction == Cardinal::north();
    if (isInternal && (isDirectionHigh || direction == Cardinal::south())) {
        const auto keyStatus = isDirectionHigh ? keyStatusAbove : keyStatusBelow;
        if (keyStatus.isEmpty()) {
            // this button state requires another level to the elevator, but the next level's keyed/keyless status could not be determined
            return false;
        }
        // assume that not-keyed means keyless
        // const auto isAccessedKeyed = keyStatus.map<bool>([&](const bool& isKeyed){return isKeyed;}).orElse(false);
        // keyless => pay-higher/pay-lower
        // keyed => ascend/descend
        if (isPayingButton(DoorEnum(sourceWall.door))) {
            if (controller.takeCharacterAction(subject) && controller.takeInventoryItem(inventory, ITEM_KEY_ELEVATOR)) {
                // happy path terminal for paying key
                return mutator.setupElevatorLevel(room.roomId, adjacentRoomIds, true, true, true, keyStatusAbove.isPresent(), keyStatusBelow.isPresent());
            } else {
                // failed action/key payment
                return false;
            }
        } else if (isMovingButton(DoorEnum(sourceWall.door))) {
            if (controller.takeCharacterAction(subject)) {
                // start moving the elevator
                // close all exterior doors,
                // assign the new interior adjacent rooms
                // determine which are doorways on the new level
                // assign the new interior/exterior doors. <- similar code from DungeonMutator
                if (!mutator.setupElevatorLevel(room.roomId, adjacentRoomIds, false, true, true, keyStatusAbove.isPresent(), keyStatusBelow.isPresent())) {
                    // failed previous setup
                    return false;
                }
                const Array<Maybe<int>, 4> nextAdjacentRoomIds = isDirectionHigh ? getAccessedRoomIdsFromAdjacentRoomIds(getAccessedRoomIdsFromAdjacentRoomIds(adjacentRoomIds, accessorAbove), accessorAbove) : getAccessedRoomIdsFromAdjacentRoomIds(getAccessedRoomIdsFromAdjacentRoomIds(adjacentRoomIds, accessorBelow), accessorBelow);
                const auto isExistingNext = nextAdjacentRoomIds.isAny([&](const Maybe<int>& nextAdjacentRoomIdMaybe){return nextAdjacentRoomIdMaybe.isPresent();});
                if (!mutator.setupElevatorLevel(room.roomId, adjacentRoomIds, true, true, true, isDirectionHigh ? isExistingNext : true, isDirectionHigh ? true : isExistingNext)) {
                    // failed next setup
                    return false;
                }
                return true;
            } else {
                // failed action payment
                return false;
            }
        }
    }


    // ------------ old code -----------
    bool isSuccess = false;
    const bool isNeighborAccessed = activation.match.dungeon.accessWallNeighbor(room, direction,
        [&](Wall& neighborWall, Room& neighbor, int neighborId) {
            int outCharacterId;
            switch (sourceWall.door) {
                case DOOR_KEEPER_INGRESS_KEYED:
                    if (controller.takeCharacterAction(subject)) {
                        if (controller.giveInventoryItem(inventory, ITEM_KEY)) {
                            sourceWall.door = DOOR_KEEPER_INGRESS_KEYLESS;
                            neighborWall.door = DOOR_KEEPER_EGRESS_KEYLESS;
                            isSuccess = true;
                        }
                    }
                    return;
                case DOOR_KEEPER_INGRESS_KEYLESS:
                    if (controller.takeCharacterAction(subject)) {
                        if (controller.takeInventoryItem(inventory, ITEM_KEY)) {
                            sourceWall.door = DOOR_KEEPER_INGRESS_KEYED;
                            neighborWall.door = DOOR_KEEPER_EGRESS_KEYED;
                            isSuccess = true;
                        }
                    }
                    return;
                default:
                    codeset.addError(CODE_KEEPER_ACTIVATION_ON_NON_INGRESS_KEEPER);
                    return;
            }
        }
    );

    codeset.addFailure(!isNeighborAccessed, CODE_INACCESSIBLE_NEIGHBORING_WALL);

    return isSuccess;
}
