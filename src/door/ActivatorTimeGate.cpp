#include "ActivatorTimeGate.hpp"
#include "Match.hpp"
#include "Room.hpp"
#include "Character.hpp"
#include "iLayout.hpp"
#include "Inventory.hpp"
#include "Player.hpp"
#include "DoorEnum.hpp"

CodeEnum ActivatorTimeGate::activate(Activation& activation) const {
    // Check if character can use keys
    CodeEnum result = CODE_PREACTIVATE_IN_ACTIVATOR;

    if (!activation.character.isMovable(result, true)) {
        return result;
    }

    activation.match.dungeon.accessWall(activation.room, activation.direction,
        [&](Cell& sourceCell) {
            Wall& sourceWall = activation.room.getWall(activation.direction);

            // Check for occupied target cell
            if (activation.match.containsOffset(activation.room.getWall(TIME_GATE_DIRECTION).cell.offset)) {
                result = CODE_OCCUPIED_TARGET_TIME_GATE_CELL;
                return;
            }

            auto& inventory = activation.player.inventory;
            int delta = 0;
            switch (activation.room.type) {
                case ROOM_TIME_GATE_TO_FUTURE:
                    delta = 1;
                    break;
                case ROOM_TIME_GATE_TO_PAST:
                    delta = -1;
                    break;
                default:
                    result = CODE_TIME_GATE_ACTIVATED_BUT_ROOM_ISNT_TIME_GATE;
                    return;
            }

            switch (sourceWall.door) {
                case DOOR_TIME_GATE_AWAKENED:
                    {
                        const bool isDeltaValid = activation.match.dungeon.rooms.access(activation.room.getDeltaTime(delta), [&](Room& room2) {
                            if (activation.match.containsOffset(activation.room.getWall(TIME_GATE_DIRECTION).cell.offset)) {
                                result = CODE_OCCUPIED_TARGET_TIME_GATE_CELL;
                                return;
                            }
                            
                            // cleanup character
                            int characterId;
                            bool wasWalled, wasFloored;
                            int2 prevFloor;
                            Cardinal prevWall = TIME_GATE_DIRECTION;
                            if (!activation.match.cleanupMovement(activation.character, activation.room, characterId, wasFloored, prevFloor, wasWalled, prevWall, result)) return;

                            // update the time gates
                            activation.room.getWall(TIME_GATE_DIRECTION).door = DOOR_TIME_GATE_DORMANT;
                            Wall& wall2 = room2.getWall(TIME_GATE_DIRECTION);
                            wall2.door = DOOR_TIME_GATE_DORMANT;
                            wall2.cell.offset = characterId;

                            // take character move
                            if (activation.character.takeMove(result)) {
                                result = CODE_SUCCESS;

                                // animate
                                const Keyframe keyframe = wasFloored
                                    // floor -> wall
                                    ? Keyframe::buildWalking(activation.time, Match::MOVE_ANIMATION_DURATION, activation.getRoomId(), prevFloor, TIME_GATE_DIRECTION)
                                    // wall -> wall
                                    : Keyframe::buildWalking(activation.time, Match::MOVE_ANIMATION_DURATION, activation.getRoomId(), prevWall, TIME_GATE_DIRECTION);
                                if(!Keyframe::insertKeyframe(Rack<Keyframe>::buildFromArray<Character::MAX_KEYFRAMES>(activation.character.keyframes), keyframe)) {
                                    // TODO: animation overflow
                                }
                                // this is the end of the movement!!!
                                return;
                            }
                        });

                        if (!isDeltaValid) {
                            result = CODE_TIME_GATE_ACTIVATED_BUT_NEIGHBOR_DNE;
                            return;
                        }
                    }

                    return;
                default:
                    result = CODE_TIME_GATE_ACTIVATED_BUT_WALL_MISSING_AWAKENED_CUBE;
                    return;
            }
        }
    );

    return result;
} 