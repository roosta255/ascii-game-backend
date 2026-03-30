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
    bool result = false;
    activation.request.access([&](RequestContext& req) {
        auto& controller = req.controller;
        auto& codeset = req.codeset;
        auto& dungeon = req.match.dungeon;
        auto& match = req.match;
        auto mutator = DungeonMutator(req.controller);
        mutator.isElevatorOverride = true;
        auto& subject = activation.character;
        auto& inventory = req.player.inventory;
        auto& room = req.room;

        Cardinal outDirection;
        if (codeset.addFailure(!activation.direction.copy(outDirection), CODE_ACTIVATION_DIRECTION_NOT_SPECIFIED)) {
            return;
        }
        const Cardinal direction = outDirection;

        if (!controller.isCharacterKeyerValidation(subject)) {
            controller.addRequestLoggedEvent(activation, LoggedEvent{
                EVENT_NOT_KEYER,
                { EventComponentKind::ROLE, (int)subject.role },
                {}, {}, -1
            });
            return;
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
                return;
            }

            const auto keyStatus = isDirectionHigh ? keyStatusAbove : keyStatusBelow;
            if (codeset.addFailure(keyStatus.isEmpty(), CODE_ELEVATOR_CANNOT_DETERMINE_KEY_STATUS)) {
                return;
            }

            const auto isMoving = keyStatus.orElse(false);
            const auto isPaying = !isMoving;

            codeset.addTable(CODE_ELEVATOR_IS_MOVING, isMoving);
            codeset.addTable(CODE_ELEVATOR_IS_PAYING, isPaying);

            if (codeset.addFailure(!controller.takeCharacterAction(subject), CODE_ELEVATOR_FAILED_TO_PAY_ACTION)) {
                controller.addRequestLoggedEvent(activation, LoggedEvent{
                    EVENT_NO_ACTIONS,
                    { EventComponentKind::ROLE, (int)subject.role },
                    {}, {}, -1
                });
                return;
            }

            if (isPaying && codeset.addFailure(!controller.takeInventoryItem(inventory, ITEM_KEY_ELEVATOR, req.time, room.roomId, req.isSkippingAnimations), CODE_ELEVATOR_FAILED_TO_PAY_KEY)) {
                controller.addRequestLoggedEvent(activation, LoggedEvent{
                    EVENT_MISSING_ITEM,
                    { EventComponentKind::ROLE, (int)subject.role },
                    {},
                    { EventComponentKind::ITEM, (int)ITEM_KEY_ELEVATOR },
                    -1
                });
                return;
            }

            if (isMoving) {
                for (const auto dir: Cardinal::getAllCardinals()) {
                    int blockingCharacterId = -1;
                    const auto doorOccupied = controller.isDoorOccupied(room.roomId, CHANNEL_CORPOREAL, dir, blockingCharacterId);
                    if (codeset.addFailure(doorOccupied, CODE_ELEVATOR_DOOR_IS_OCCUPIED_BY_CHARACTER)){
                        RoleEnum occRole = ROLE_EMPTY;
                        CodeEnum charError = CODE_UNKNOWN_ERROR;
                        match.getCharacter(blockingCharacterId, charError).access([&](Character& c) { occRole = c.role; });
                        controller.addRequestLoggedEvent(activation, LoggedEvent{
                            EVENT_OCCUPIED_DOORWAY,
                            { EventComponentKind::ROLE, (int)occRole },
                            {},
                            { EventComponentKind::DOOR, (int)room.getWall(dir).door },
                            dir.getIndex()
                        });
                        return;
                    }
                }
            }

            const auto directUpdateElevatorProperties = DungeonMutator::ElevatorProperties
                { .connectedRoomIds = isDirectionHigh ? aboveAdjacentRoomIds : belowAdjacentRoomIds,
                  .isPaid = true
                };

            const auto selfUpdateElevatorProperties = DungeonMutator::ElevatorProperties
                { .connectedRoomIds = startingAdjacentRoomIds,
                  .isPaid = true
                };

            const auto subsequentAdjacentRooms = getAccessedRoomIdsFromAdjacentRoomIds(directUpdateElevatorProperties.connectedRoomIds, isDirectionHigh);
            const auto subsequentKeyStatus = getKeyStatusOfLevel(subsequentAdjacentRooms);
            const auto isSubsequentLevelExisting = subsequentKeyStatus.isPresent();
            const auto isSubsequentLevelPaid = subsequentKeyStatus.orElse(false);

            if (codeset.addFailure(!mutator.setupElevatorLevel(room.roomId, directUpdateElevatorProperties, isMoving, isDirectionHigh ? isSubsequentLevelExisting : true, isDirectionHigh ? true : isSubsequentLevelExisting, isDirectionHigh ? isSubsequentLevelPaid : true, isDirectionHigh ? true : isSubsequentLevelPaid), CODE_ELEVATOR_FAILED_TO_SETUP_ELEVATOR_DIRECT)) {
                return;
            }

            if (isMoving && codeset.addFailure(!mutator.setupElevatorLevel(room.roomId, selfUpdateElevatorProperties, false, keyStatusAbove.isPresent(), keyStatusBelow.isPresent(), false, false), CODE_ELEVATOR_FAILED_TO_SETUP_ELEVATOR_SELF)) {
                return;
            }

            if (isPaying && codeset.addFailure(!mutator.setupElevatorLevel(room.roomId, selfUpdateElevatorProperties, true, keyStatusAbove.isPresent(), keyStatusBelow.isPresent(), isDirectionHigh ? true : keyStatusAbove.orElse(false), isDirectionHigh ? keyStatusBelow.orElse(false) : true), CODE_ELEVATOR_FAILED_TO_SETUP_ELEVATOR_SELF)) {
                return;
            }

            controller.addLoggedEvent(activation, room.roomId, LoggedEvent{
                EVENT_ELEVATOR,
                { EventComponentKind::ROLE, (int)subject.role },
                {},
                {},
                (int)direction
            });
            result = true;
            return;
        } else {
            // call elevator button
        }
    });
    return result;
}
