#include "ActivatorLightningRod.hpp"
#include "ActivatorTimeGate.hpp"
#include "Character.hpp"
#include "Codeset.hpp"
#include "Dungeon.hpp"
#include "DungeonMutator.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "RoleEnum.hpp"

DungeonMutator::DungeonMutator(MatchController& controller)
    : dungeon(controller.match.dungeon), codeset(controller.codeset), controller(controller) {}

Pointer<Room> DungeonMutator::getRoom(const int& roomId) {
    auto roomPtr = dungeon.rooms.getPointer(roomId);
    if (roomPtr.isEmpty()) {
        codeset.addError(CODE_DUNGEON_MUTATOR_GET_ROOM_REQUIRES_VALID_ROOM_ID);
    }
    return roomPtr;
}

bool DungeonMutator::setDoor(const int& roomId, Cardinal dir, DoorEnum type) {
    return getRoom(roomId).map<bool>([&](Room& room){
            Wall& wall = room.getWall(dir);
            if (wall.door != DOOR_WALL && wall.door != DOOR_DOORWAY) {
               codeset.addError(CODE_DUNGEON_MUTATOR_SET_DOOR_REQUIRES_DEFAULT_DOOR_TYPE);
                return false;
            }
            wall.door = type;
            return true;
        }).orElse(false);
}

bool DungeonMutator::setRoom(const int& roomId, RoomEnum type) {
    return getRoom(roomId).map<bool>([&](Room& room){
            if (room.type != ROOM_RECT_4_x_5) {
                codeset.addError(CODE_DUNGEON_MUTATOR_SET_ROOM_REQUIRES_DEFAULT_ROOM_TYPE);
                return false;
            }
            room.type = type;
            return true;
        }).orElse(false);
}

bool DungeonMutator::setSharedDoor(
    const int& room1Id,
    Cardinal dir,
    DoorEnum door1,
    DoorEnum door2
) {
    return getRoom(room1Id).map<bool>([&](Room& room1){
            Wall& wall1 = room1.getWall(dir);
            if (wall1.door != DOOR_WALL && wall1.door != DOOR_DOORWAY) {
                codeset.addError(CODE_DUNGEON_MUTATOR_SET_SHARED_DOOR_REQUIRES_DEFAULT_DOOR_TYPE_FROM_DOOR_1);
                return false;
            }
            return getRoom(wall1.adjacent).map<bool>([&](Room& room2){
                    Wall& wall2 = room2.getWall(dir.getFlip());
                    if (wall2.door != DOOR_WALL && wall2.door != DOOR_DOORWAY) {
                        codeset.addError(CODE_DUNGEON_MUTATOR_SET_SHARED_DOOR_REQUIRES_DEFAULT_DOOR_TYPE_FROM_DOOR_2);
                        return false;
                    }
                    wall1.door = door1;
                    wall2.door = door2;
                    return true;
                }).orElse(false);
        }).orElse(false);
}

bool DungeonMutator::setSharedShaftAbove(
    const int& room1Id,
    Cardinal dir,
    DoorEnum door1,
    DoorEnum door2
) {
    return getRoom(room1Id).map<bool>([&](Room& room1){
            Wall& wall1 = room1.getWall(dir);
            if (wall1.door != DOOR_WALL && wall1.door != DOOR_DOORWAY) {
                codeset.addError(CODE_DUNGEON_MUTATOR_SET_SHARED_SHAFT_REQUIRES_DEFAULT_DOOR_TYPE_FROM_DOOR_1);
                return false;
            }
            return getRoom(room1.above).map<bool>([&](Room& room2){
                    Wall& wall2 = room2.getWall(dir);
                    if (wall2.door != DOOR_WALL && wall2.door != DOOR_DOORWAY) {
                        codeset.addError(CODE_DUNGEON_MUTATOR_SET_SHARED_DOOR_REQUIRES_DEFAULT_DOOR_TYPE_FROM_DOOR_2);
                        return false;
                    }
                    wall1.door = door1;
                    wall2.door = door2;
                    return true;
                }).orElse(false);
        }).orElse(false);
}

bool DungeonMutator::setupDoorway (const int& roomId, const Cardinal dir) {
    return !codeset.addFailure(!setSharedDoor(roomId, dir, DOOR_DOORWAY, DOOR_DOORWAY), CODE_GENERATOR_UTILITY_FAILED_TO_SETUP_DOORWAY);
}

bool DungeonMutator::setup2x5Room(const int& roomId) {
    return setRoom(roomId, ROOM_RECT_2_x_5);
}

bool DungeonMutator::setup3x3Room(const int& roomId) {
    return setRoom(roomId, ROOM_RECT_3_x_3);
}

bool DungeonMutator::setup4x1Room(const int& roomId) {
    return setRoom(roomId, ROOM_RECT_4_x_1);
}

bool DungeonMutator::setupJailer (const int& roomId, const Cardinal dir, const bool isKeyed) {
    return !codeset.addFailure(!setSharedDoor(roomId, dir, isKeyed ? DOOR_JAILER_INGRESS_KEYED : DOOR_JAILER_INGRESS_KEYLESS, isKeyed ? DOOR_JAILER_EGRESS_KEYED : DOOR_JAILER_EGRESS_KEYLESS), CODE_GENERATOR_UTILITY_FAILED_TO_SETUP_JAILER);
}

bool DungeonMutator::setupKeeper (const int& roomId, const Cardinal dir, const bool isKeyed) {
    return !codeset.addFailure(!setSharedDoor(roomId, dir, isKeyed ? DOOR_KEEPER_INGRESS_KEYED : DOOR_KEEPER_INGRESS_KEYLESS, isKeyed ? DOOR_KEEPER_EGRESS_KEYED : DOOR_KEEPER_EGRESS_KEYLESS), CODE_GENERATOR_UTILITY_FAILED_TO_SETUP_KEEPER);
}

bool DungeonMutator::setupLadderUp(const int& roomId, const Cardinal dir) {
    return setSharedShaftAbove(roomId, dir, DOOR_LADDER_1_BOTTOM, DOOR_LADDER_1_TOP);
}

bool DungeonMutator::setupLightningRodRoom(const int& roomId, const bool isCubed, const bool isAwakened) {
    return setRoom(roomId, ROOM_LIGHTNING_ROD)
        && setDoor(roomId, ActivatorLightningRod::LIGHTNING_ROD_WALL, !isCubed ? DOOR_LIGHTNING_ROD_EMPTY : isAwakened ? DOOR_LIGHTNING_ROD_AWAKENED : DOOR_LIGHTNING_ROD_DORMANT);
}

bool DungeonMutator::setupPoleUp(const int& roomId, const Cardinal dir) {
    return setSharedShaftAbove(roomId, dir, DOOR_POLE_1_BOTTOM, DOOR_POLE_1_TOP);
}

bool DungeonMutator::setupPowerGeneratorRoom(const int& roomId) {
    return setRoom(roomId, ROOM_POWER_GENERATOR);
}

bool DungeonMutator::setupShifter (const int& roomId, const Cardinal dir, const bool isKeyed) {
    return !codeset.addFailure(!setSharedDoor(roomId, dir, isKeyed ? DOOR_SHIFTER_INGRESS_KEYED : DOOR_SHIFTER_INGRESS_KEYLESS, isKeyed ? DOOR_SHIFTER_EGRESS_KEYED : DOOR_SHIFTER_EGRESS_KEYLESS), CODE_GENERATOR_UTILITY_FAILED_TO_SETUP_SHIFTER);
}

bool DungeonMutator::setupTimeGateRoomToFuture(const int& roomId, const bool isCubed, const bool isAwakened) {
    if (!setRoom(roomId, ROOM_TIME_GATE_TO_FUTURE) || !setDoor(roomId, ActivatorTimeGate::TIME_GATE_DIRECTION, !isCubed ? DOOR_TIME_GATE_EMPTY : isAwakened ? DOOR_TIME_GATE_AWAKENED : DOOR_TIME_GATE_DORMANT))
        return false;
    return getRoom(roomId).map<bool>([&](Room& room1){
        const auto futureRoomId = room1.posterior;
        return setRoom(futureRoomId, ROOM_TIME_GATE_TO_PAST) && setDoor(futureRoomId, ActivatorTimeGate::TIME_GATE_DIRECTION, DOOR_TIME_GATE_EMPTY);
    }).orElse(false);
}

bool DungeonMutator::setupTogglerBlue (const int& roomId, const Cardinal dir) {
    return !codeset.addFailure(!setSharedDoor(roomId, dir, DOOR_TOGGLER_BLUE_CLOSED, DOOR_TOGGLER_BLUE_CLOSED), CODE_GENERATOR_UTILITY_FAILED_TO_SETUP_TOGGLER_BLUE);
}

bool DungeonMutator::setupTogglerOrange (const int& roomId, const Cardinal dir) {
    return !codeset.addFailure(!setSharedDoor(roomId, dir, DOOR_TOGGLER_ORANGE_OPEN, DOOR_TOGGLER_ORANGE_OPEN), CODE_GENERATOR_UTILITY_FAILED_TO_SETUP_TOGGLER_ORANGE);
}

bool DungeonMutator::setupTogglerSwitch (const int& roomId, int& outCharacterId, int& outFloorId) {
    return !codeset.addFailure(!controller.allocateCharacterToFloor(roomId, CHANNEL_CORPOREAL, [&](Character& character){
        character.role = ROLE_TOGGLER;
        character.visibility = ~0x0;
    }, outCharacterId, outFloorId), CODE_DUNGEON_MUTATOR_FAILED_TO_SETUP_TOGGLER_SWITCH);
}
