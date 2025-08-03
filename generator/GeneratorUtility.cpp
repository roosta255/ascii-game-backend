#include "build_room_map.hpp"
#include "DoorEnum.hpp"
#include "GeneratorUtility.hpp"
#include "iLayout.hpp"

GeneratorUtility::GeneratorUtility (Array<Room, DUNGEON_ROOM_COUNT>& roomsIn, const iLayout& layoutIn):
    rooms(roomsIn), layout(&layoutIn)
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

bool GeneratorUtility::setupKeeper (const int4& coord, const Cardinal dir, const bool isKeyed) {
    return accessRoomWall(coord, dir, [&](Room&, Wall& wall1, Wall& wall2, Room&){
        wall1.door = isKeyed ? DOOR_KEEPER_INGRESS_KEYED : DOOR_KEEPER_INGRESS_KEYLESS;
        wall2.door = isKeyed ? DOOR_KEEPER_EGRESS_KEYED : DOOR_KEEPER_EGRESS_KEYLESS;
    });
}

bool GeneratorUtility::setupShifter (const int4& coord, const Cardinal dir, const bool isKeyed) {
    return accessRoomWall(coord, dir, [&](Room&, Wall& wall1, Wall& wall2, Room&){
        wall1.door = isKeyed ? DOOR_SHIFTER_INGRESS_KEYED : DOOR_SHIFTER_INGRESS_KEYLESS;
        wall2.door = isKeyed ? DOOR_SHIFTER_EGRESS_KEYED : DOOR_SHIFTER_EGRESS_KEYLESS;
    });
}

bool GeneratorUtility::setupTogglerOrange (const int4& coord, const Cardinal dir) {
    return accessRoomWall(coord, dir, [&](Room&, Wall& wall1, Wall& wall2, Room&){
        wall1.door = DOOR_TOGGLER_ORANGE_OPEN;
        wall2.door = DOOR_TOGGLER_ORANGE_OPEN;
    });
}

bool GeneratorUtility::setupTogglerBlue (const int4& coord, const Cardinal dir) {
    return accessRoomWall(coord, dir, [&](Room&, Wall& wall1, Wall& wall2, Room&){
        wall1.door = DOOR_TOGGLER_BLUE_CLOSED;
        wall2.door = DOOR_TOGGLER_BLUE_CLOSED;
    });
}

bool GeneratorUtility::setupDoorway (const int4& coord, const Cardinal dir) {
    return accessRoomWall(coord, dir, [&](Room&, Wall& wall1, Wall& wall2, Room&){
        wall1.door = DOOR_DOORWAY;
        wall2.door = DOOR_DOORWAY;
    });
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
