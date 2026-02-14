#include "ActivatorLightningRod.hpp"
#include "ActivatorTimeGate.hpp"
#include "build_room_map.hpp"
#include "Codeset.hpp"
#include "DoorEnum.hpp"
#include "Dungeon.hpp"
#include "DungeonAuthor.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Room.hpp"
#include "RoomEnum.hpp"
#include "iLayout.hpp"

DungeonAuthor::DungeonAuthor (MatchController& controller, const iLayout& layoutIn):
    dungeon(controller.match.dungeon), layout(&layoutIn), codeset(controller.codeset), mutator(controller), controller(controller)
{
    size = build_room_map(dungeon.rooms, *layout, mapping);
}

Maybe<int> DungeonAuthor::getRoomId(const int4& coord) {
    auto found = mapping.find(coord);
    if (found == mapping.end()) {
        codeset.addError(CODE_DUNGEON_AUTHOR_FAILED_MAP_COORDINATE);
        return Maybe<int>::empty();
    }
    return found->second.map<int>([&](Room& room){return room.roomId;});
}

bool DungeonAuthor::setup2x5Room(const int4& coord) {
    return getRoomId(coord).map<bool>([&](const int& roomId){
            return mutator.setup2x5Room(roomId);
        }).orElse(false);
}

bool DungeonAuthor::setup3x3Room(const int4& coord) {
    return getRoomId(coord).map<bool>([&](const int& roomId){
            return mutator.setup3x3Room(roomId);
        }).orElse(false);
}

bool DungeonAuthor::setup4x1Room(const int4& coord) {
    return getRoomId(coord).map<bool>([&](const int& roomId){
            return mutator.setup4x1Room(roomId);
        }).orElse(false);
}

bool DungeonAuthor::setupDoorway (const int4& coord, const Cardinal dir) {
    return getRoomId(coord).map<bool>([&](const int& roomId){
            return mutator.setupDoorway(roomId, dir);
        }).orElse(false);
}

bool DungeonAuthor::setupJailer (const int4& coord, const Cardinal dir, const bool isKeyed) {
    return getRoomId(coord).map<bool>([&](const int& roomId){
            return mutator.setupJailer(roomId, dir, isKeyed);
        }).orElse(false);
}

bool DungeonAuthor::setupKeeper (const int4& coord, const Cardinal dir, const bool isKeyed) {
    return getRoomId(coord).map<bool>([&](const int& roomId){
            return mutator.setupKeeper(roomId, dir, isKeyed);
        }).orElse(false);
}

bool DungeonAuthor::setupLadderUp(const int4& coord, const Cardinal dir) {
    return getRoomId(coord).map<bool>([&](const int& roomId){
            return mutator.setupLadderUp(roomId, dir);
        }).orElse(false);
}

bool DungeonAuthor::setupPoleUp(const int4& coord, const Cardinal dir) {
    return getRoomId(coord).map<bool>([&](const int& roomId){
            return mutator.setupPoleUp(roomId, dir);
        }).orElse(false);
}

bool DungeonAuthor::setupShifter (const int4& coord, const Cardinal dir, const bool isKeyed) {
    return getRoomId(coord).map<bool>([&](const int& roomId){
            return mutator.setupShifter(roomId, dir, isKeyed);
        }).orElse(false);
}

bool DungeonAuthor::setupTimeGateRoomToFuture(const int4& coord, const bool isCubed, const bool isAwakened) {
    return getRoomId(coord).map<bool>([&](const int& roomId){
            return mutator.setupTimeGateRoomToFuture(roomId, isCubed, isAwakened);
        }).orElse(false);
}

bool DungeonAuthor::setupTogglerBlue (const int4& coord, const Cardinal dir) {
    return getRoomId(coord).map<bool>([&](const int& roomId){
            return mutator.setupTogglerBlue(roomId, dir);
        }).orElse(false);
}

bool DungeonAuthor::setupTogglerOrange (const int4& coord, const Cardinal dir) {
    return getRoomId(coord).map<bool>([&](const int& roomId){
            return mutator.setupTogglerOrange(roomId, dir);
        }).orElse(false);
}

bool DungeonAuthor::setupTogglerSwitch (const int4& coord, int& outCharacterId, int& outFloorId) {
    return getRoomId(coord).map<bool>([&](const int& roomId){
            return mutator.setupTogglerSwitch(roomId, outCharacterId, outFloorId);
        }).orElse(false);
}

bool DungeonAuthor::setupHorizontalWalls(std::initializer_list<int> row, int y, int z) {
    if (row.size() != size[0]) {
        return false;
    }

    auto it = row.begin();
    for (int t = 0; t < size[3]; ++t) {
        for (int x = 0; x < size[0]; x++, it++) {
            const int4 coord = {x, y, z, t};
            auto found = mapping.find(coord);
            if (found == mapping.end()) {
                continue;
            }

            bool isInnerMissing;
            const bool isOuterMissing = !found->second.access([&](Room& roomA) {
                Wall& wallA = roomA.getWall(Cardinal::north());
                isInnerMissing = !layout->getWallNeighbor(dungeon.rooms, roomA, Cardinal::north()).access([&](Room& roomB) {
                    Wall& wallB = roomB.getWall(Cardinal::south());
                    wallA.door = *it;
                    wallB.door = *it;
                });
            });
        }
    }

    return true;
}

bool DungeonAuthor::setupVerticalWalls(std::initializer_list<int> row, int y, int z) {
    if (row.size() != size[0] - 1) {
        return false;
    }

    auto it = row.begin();
    for (int t = 0; t < size[3]; ++t) {
        for (int x = 0; x < size[0] - 1; ++x, ++it) {
            int4 coord = {x, y, z, t};
            auto found = mapping.find(coord);
            if (found == mapping.end()) {
                continue;
            }

            found->second.access([&](Room& roomA) {
                Wall& wallEast = roomA.getWall(Cardinal::east());

                layout->getWallNeighbor(dungeon.rooms, roomA, Cardinal::east()).access([&](Room& roomB) {
                    Wall& wallWest = roomB.getWall(Cardinal::west());
                    wallEast.door = *it;
                    wallWest.door = *it;
                });
            });
        }
    }

    return true;
}

bool DungeonAuthor::setupLightningRodRoom(const int4& coord, const bool isCubed, const bool isAwakened) {
    return getRoomId(coord).map<bool>([&](const int& roomId){
            return mutator.setupLightningRodRoom(roomId, isCubed, isAwakened);
        }).orElse(false);
}

bool DungeonAuthor::setupPowerGeneratorRoom(const int4& coord) {
    return getRoomId(coord).map<bool>([&](const int& roomId){
            return mutator.setupPowerGeneratorRoom(roomId);
        }).orElse(false);
}
