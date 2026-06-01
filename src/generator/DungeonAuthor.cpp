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
    mutator.setCoordinateResolver([this](const int4& c){ return this->getRoomId(c); });
}

Maybe<int> DungeonAuthor::getRoomId(const int4& coord) {
    auto found = mapping.find(coord);
    if (found == mapping.end()) {
        codeset.addError(CODE_DUNGEON_AUTHOR_FAILED_MAP_COORDINATE);
        return Maybe<int>::empty();
    }
    return found->second.map<int>([&](Room& room){return room.roomId;});
}

bool DungeonAuthor::setupChest(const RoomRef& room, const DungeonMutator::ChestSpec& spec) {
    return mutator.setupChest(room, spec);
}

bool DungeonAuthor::setup2x5Room(const RoomRef& room) {
    return mutator.setup2x5Room(room);
}

bool DungeonAuthor::setup3x3Room(const RoomRef& room) {
    return mutator.setup3x3Room(room);
}

bool DungeonAuthor::setup4x1Room(const RoomRef& room) {
    return mutator.setup4x1Room(room);
}

bool DungeonAuthor::setupDoorway(const RoomRef& room, const Cardinal dir) {
    return mutator.setupDoorway(room, dir);
}

bool DungeonAuthor::setupExitDoor(const RoomRef& room, const Cardinal dir) {
    return mutator.setupExitDoor(room, dir);
}

bool DungeonAuthor::setupJailer(const RoomRef& room, const Cardinal dir, const bool isKeyed) {
    return mutator.setupJailer(room, dir, isKeyed);
}

bool DungeonAuthor::setupKeeper(const RoomRef& room, const Cardinal dir, const bool isKeyed) {
    return mutator.setupKeeper(room, dir, isKeyed);
}

bool DungeonAuthor::setupLadderUp(const RoomRef& bottomRoom, const Cardinal dir) {
    return mutator.setupLadderUp(bottomRoom, dir);
}

bool DungeonAuthor::setupPoleUp(const RoomRef& bottomRoom, const Cardinal dir) {
    return mutator.setupPoleUp(bottomRoom, dir);
}

bool DungeonAuthor::setupCovenantDoor(const RoomRef& room, const Cardinal dir) {
    return mutator.setupCovenantDoor(room, dir);
}

bool DungeonAuthor::setupShifter(const RoomRef& room, const Cardinal dir, const bool isKeyed) {
    return mutator.setupShifter(room, dir, isKeyed);
}

bool DungeonAuthor::setupTimeGateRoomToFuture(const RoomRef& room, const bool isCubed, const bool isAwakened) {
    return mutator.setupTimeGateRoomToFuture(room, isCubed, isAwakened);
}

bool DungeonAuthor::setupTogglerBlue(const RoomRef& room, const Cardinal dir) {
    return mutator.setupTogglerBlue(room, dir);
}

bool DungeonAuthor::setupTogglerOrange(const RoomRef& room, const Cardinal dir) {
    return mutator.setupTogglerOrange(room, dir);
}

bool DungeonAuthor::setupTogglerSwitch(const RoomRef& room, int& outCharacterId, int& outFloorId) {
    return mutator.setupTogglerSwitch(room, outCharacterId, outFloorId);
}

bool DungeonAuthor::setupSacramentForgiveness(const RoomRef& room, int& outCharacterId, int& outFloorId) {
    return mutator.setupSacramentForgiveness(room, outCharacterId, outFloorId);
}

bool DungeonAuthor::setupBuilderStartingRoomId(int builderIndex, const RoomRef& room) {
    return mutator.setBuilderStartingRoomId(builderIndex, room);
}

bool DungeonAuthor::setupElevatorColumn(int elevatorRoomId, const std::vector<int>& columnRoomIds, int paidIndex) {
    const auto EMPTY = Maybe<int>::empty();
    std::vector<DungeonMutator::ElevatorProperties> props;
    props.reserve(columnRoomIds.size());
    for (int i = 0; i < (int)columnRoomIds.size(); ++i) {
        props.push_back({
            .connectedRoomIds = Array<Maybe<int>, 4>({EMPTY, EMPTY, Maybe<int>(columnRoomIds[i]), EMPTY}),
            .isPaid = i == paidIndex
        });
    }
    return mutator.setupElevatorColumn(elevatorRoomId, Rack<DungeonMutator::ElevatorProperties>(props));
}

bool DungeonAuthor::setupHorizontalWalls(std::vector<DoorEnum> row, int y, int z) {
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

bool DungeonAuthor::setupVerticalWalls(std::vector<DoorEnum> row, int y, int z) {
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

bool DungeonAuthor::setupLightningRodRoom(const RoomRef& room, const bool isCubed, const bool isAwakened) {
    return mutator.setupLightningRodRoom(room, isCubed, isAwakened);
}

bool DungeonAuthor::setupPowerGeneratorRoom(const RoomRef& room) {
    return mutator.setupPowerGeneratorRoom(room);
}
