#include "build_room_map.hpp"
#include "DoorEnum.hpp"
#include "GeneratorUtility.hpp"
#include "iLayout.hpp"

#include <drogon/drogon.h>

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
        if(failure) {
            LOG_ERROR << "GeneratorUtility{size<" << size[0] << "," << size[1] << "," << size[2] << "," << size[3] << ">}::accessRoomWall({coord<" << coord[0] << "," << coord[1] << "," << coord[2] << "," << coord[3] << ">, dir: " << dir.getIndex() << "} failed due to missing room on the opposite wall.";
        }
    })) {
        LOG_ERROR << "GeneratorUtility{size<" << size[0] << "," << size[1] << "," << size[2] << "," << size[3] << ">}::accessRoomWall({coord<" << coord[0] << "," << coord[1] << "," << coord[2] << "," << coord[3] << ">, dir: " << dir.getIndex() << "} failed due to missing coordinate.";
    }
    return success;
}

bool GeneratorUtility::setupHorizontalWalls(std::initializer_list<int> row, int y, int z) {
    if (row.size() != size[0]) {
        LOG_ERROR << "GeneratorUtility{size<" << size[0] << "," << size[1] << "," << size[2] << "," << size[3] << ">}::setupHorizontalWalls({size: " << row.size() << "}, " << y << ", " << z << ") failed due to row size mismatch.";
        return false;
    }

    auto it = row.begin();
    for (int t = 0; t < size[3]; ++t) {
        for (int x = 0; x < size[0]; x++, it++) {
            const int4 coord = {x, y, z, t};
            auto found = mapping.find(coord);
            if (found == mapping.end()) {
                LOG_ERROR << "GeneratorUtility{size<" << size[0] << "," << size[1] << "," << size[2] << "," << size[3] << ">}::setupHorizontalWalls({size: " << row.size() << "}, " << y << ", " << z << ").for(x: " << x << ") is missing mapping for coord<" << coord[0] << "," << coord[1] << "," << coord[2] << "," << coord[3] << ">";
                continue;
            }

            bool isInnerMissing;
            const bool isOuterMissing = !found->second.access([&](Room& roomA) {

                Wall& wallA = roomA.getWall(Cardinal::north());

                isInnerMissing = !layout->getWallNeighbor(rooms, roomA, Cardinal::north()).access([&](Room& roomB) {
                    Wall& wallB = roomB.getWall(Cardinal::south());
                    wallA.door = *it;
                    wallB.door = *it;
                    // LOG_DEBUG << "GeneratorUtility{size<" << size[0] << "," << size[1] << "," << size[2] << "," << size[3] << ">}::setupHorizontalWalls({size: " << row.size() << "}, " << y << ", " << z << ").for(x: " << x << ") successfully set coord<" << coord[0] << "," << coord[1] << "," << coord[2] << "," << coord[3] << ">, offsets: [" << roomA.getOffset(rooms) << ", " << roomB.getOffset(rooms) << "]";
                });
            });

            if (isInnerMissing || isOuterMissing) {
                // LOG_DEBUG << "GeneratorUtility{size<" << size[0] << "," << size[1] << "," << size[2] << "," << size[3] << ">}::setupHorizontalWalls({size: " << row.size() << "}, " << y << ", " << z << ").for(x: " << x << ") is missing room for coord<" << coord[0] << "," << coord[1] << "," << coord[2] << "," << coord[3] << ">" << (isOuterMissing ? "outer wall, possibly also inner." : "inner wall, but outer is present.");
            }
        }
    }

    return true;
}

bool GeneratorUtility::setupVerticalWalls(std::initializer_list<int> row, int y, int z) {
    if (row.size() != size[0] - 1) {
        LOG_ERROR << "GeneratorUtility{size<" << size[0] << "," << size[1] << "," << size[2] << "," << size[3] << ">}::setupVerticalWalls({size: " << row.size() << "}, " << y << ", " << z << ") failed due to row size mismatch.";
        return false;
    }

    auto it = row.begin();
    for (int t = 0; t < size[3]; ++t) {
        for (int x = 0; x < size[0] - 1; ++x, ++it) {
            int4 coord = {x, y, z, t};
            auto found = mapping.find(coord);
            if (found == mapping.end()) {
                LOG_ERROR << "GeneratorUtility{size<" << size[0] << "," << size[1] << "," << size[2] << "," << size[3] << ">}::setupVerticalWalls({size: " << row.size() << "}, " << y << ", " << z << ").for(x: " << x << ") is missing mapping for coord<" << coord[0] << "," << coord[1] << "," << coord[2] << "," << coord[3] << ">";
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
