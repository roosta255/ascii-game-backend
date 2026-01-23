#include "build_room_map.hpp"
#include "Codeset.hpp"
#include "DoorEnum.hpp"
#include "GeneratorUtility.hpp"
#include "Room.hpp"
#include "RoomEnum.hpp"
#include "iLayout.hpp"

GeneratorUtility::GeneratorUtility (Array<Room, DUNGEON_ROOM_COUNT>& roomsIn, const iLayout& layoutIn, Codeset& codeset):
    rooms(roomsIn), layout(&layoutIn), codeset(codeset)
{
    size = build_room_map(roomsIn, *layout, mapping);
}

Pointer<Room> GeneratorUtility::getRoom(const int4& coord) {
    auto found = mapping.find(coord);
    if (found == mapping.end()) {
        return Pointer<Room>::empty();
    }
    return found->second;
}

bool GeneratorUtility::setupJailer (const int4& coord, const Cardinal dir, const bool isKeyed) {
    return codeset.addSuccessElseFailure(accessRoomWall(coord, dir, [&](Room&, Wall& wall1, Wall& wall2, Room&){
        wall1.door = isKeyed ? DOOR_JAILER_INGRESS_KEYED : DOOR_JAILER_INGRESS_KEYLESS;
        wall2.door = isKeyed ? DOOR_JAILER_EGRESS_KEYED : DOOR_JAILER_EGRESS_KEYLESS;
    }), CODE_GENERATOR_UTILITY_FAILED_TO_SETUP_JAILER);
}

bool GeneratorUtility::setupKeeper (const int4& coord, const Cardinal dir, const bool isKeyed) {
    return codeset.addSuccessElseFailure(accessRoomWall(coord, dir, [&](Room&, Wall& wall1, Wall& wall2, Room&){
        wall1.door = isKeyed ? DOOR_KEEPER_INGRESS_KEYED : DOOR_KEEPER_INGRESS_KEYLESS;
        wall2.door = isKeyed ? DOOR_KEEPER_EGRESS_KEYED : DOOR_KEEPER_EGRESS_KEYLESS;
    }), CODE_GENERATOR_UTILITY_FAILED_TO_SETUP_KEEPER);
}

bool GeneratorUtility::setupShifter (const int4& coord, const Cardinal dir, const bool isKeyed) {
    return codeset.addSuccessElseFailure(accessRoomWall(coord, dir, [&](Room&, Wall& wall1, Wall& wall2, Room&){
        wall1.door = isKeyed ? DOOR_SHIFTER_INGRESS_KEYED : DOOR_SHIFTER_INGRESS_KEYLESS;
        wall2.door = isKeyed ? DOOR_SHIFTER_EGRESS_KEYED : DOOR_SHIFTER_EGRESS_KEYLESS;
    }), CODE_GENERATOR_UTILITY_FAILED_TO_SETUP_SHIFTER);
}

bool GeneratorUtility::setupTogglerOrange (const int4& coord, const Cardinal dir) {
    return codeset.addSuccessElseFailure(accessRoomWall(coord, dir, [&](Room&, Wall& wall1, Wall& wall2, Room&){
        wall1.door = DOOR_TOGGLER_ORANGE_OPEN;
        wall2.door = DOOR_TOGGLER_ORANGE_OPEN;
    }), CODE_GENERATOR_UTILITY_FAILED_TO_SETUP_TOGGLER_ORANGE);
}

bool GeneratorUtility::setupTogglerBlue (const int4& coord, const Cardinal dir) {
    return codeset.addSuccessElseFailure(accessRoomWall(coord, dir, [&](Room&, Wall& wall1, Wall& wall2, Room&){
        wall1.door = DOOR_TOGGLER_BLUE_CLOSED;
        wall2.door = DOOR_TOGGLER_BLUE_CLOSED;
    }), CODE_GENERATOR_UTILITY_FAILED_TO_SETUP_TOGGLER_BLUE);
}

bool GeneratorUtility::setupDoorway (const int4& coord, const Cardinal dir) {
    return codeset.addSuccessElseFailure(accessRoomWall(coord, dir, [&](Room&, Wall& wall1, Wall& wall2, Room&){
        wall1.door = DOOR_DOORWAY;
        wall2.door = DOOR_DOORWAY;
    }), CODE_GENERATOR_UTILITY_FAILED_TO_SETUP_DOORWAY);
}

bool GeneratorUtility::accessRoomWall (const int4& coord, const Cardinal dir, std::function<void(Room&, Wall&, Wall&, Room&)> consumer) {
    bool success = false;
    if (!getRoom(coord).access([&](Room& room1){
        bool failure = !layout->getWallNeighbor(rooms, room1, dir).access([&](Room& room2) {
            success = true;
            consumer(room1, room1.getWall(dir), room2.getWall(dir.getFlip()), room2); 
        });
    })) {
    }
    return success;
}

bool GeneratorUtility::setupHorizontalWalls(std::initializer_list<int> row, int y, int z) {
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
                isInnerMissing = !layout->getWallNeighbor(rooms, roomA, Cardinal::north()).access([&](Room& roomB) {
                    Wall& wallB = roomB.getWall(Cardinal::south());
                    wallA.door = *it;
                    wallB.door = *it;
                });
            });
        }
    }

    return true;
}

bool GeneratorUtility::setupVerticalWalls(std::initializer_list<int> row, int y, int z) {
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

                layout->getWallNeighbor(rooms, roomA, Cardinal::east()).access([&](Room& roomB) {
                    Wall& wallWest = roomB.getWall(Cardinal::west());
                    wallEast.door = *it;
                    wallWest.door = *it;
                });
            });
        }
    }

    return true;
}

bool GeneratorUtility::setup4x1Room(const int4& coord) {
    bool success = false;
    getRoom(coord).access([&](Room& room){
        room.setType(ROOM_RECT_4_x_1);
        success = true;
    });
    return success;
}

bool GeneratorUtility::setup2x5Room(const int4& coord) {
    bool success = false;
    getRoom(coord).access([&](Room& room){
        room.setType(ROOM_RECT_2_x_5);
        success = true;
    });
    return success;
}

bool GeneratorUtility::setup3x3Room(const int4& coord) {
    bool success = false;
    getRoom(coord).access([&](Room& room){
        room.setType(ROOM_RECT_3_x_3);
        success = true;
    });
    return success;
}

bool GeneratorUtility::setupLightningRodRoom(const int4& coord) {
    bool success = false;
    getRoom(coord).access([&](Room& room){
        room.setType(ROOM_LIGHTNING_ROD);
        success = true;
    });
    return success;
}

bool GeneratorUtility::setupPowerGeneratorRoom(const int4& coord) {
    bool success = false;
    getRoom(coord).access([&](Room& room){
        room.setType(ROOM_POWER_GENERATOR);
        success = true;
    });
    return success;
}

bool GeneratorUtility::setupAdjacencyPointers() {
    bool isSuccess = false;
    for (auto& room: rooms) {
        for (const auto& dir: Cardinal::getAllCardinals()) {
            codeset.addSuccess(layout->getWallNeighbor(rooms, room, dir, room.getWall(dir).adjacent));
        }
        codeset.addSuccess(layout->getDepthDelta(rooms, room, -1, room.below));
        codeset.addSuccess(layout->getDepthDelta(rooms, room, 1, room.above));
        codeset.addSuccess(layout->getTimeDelta(rooms, room, -1, room.anterior));
        codeset.addSuccess(layout->getTimeDelta(rooms, room, 1, room.posterior));
    }
    return isSuccess;
}