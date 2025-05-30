#pragma once

#include "Array.hpp"
#include "Cardinal.hpp"
#include "DUNGEON_ROOM_COUNT.hpp"
#include "Room.hpp"
// provides entrance
// provides neighbors

class iLayout {
public:
    virtual Room& getEntrance(Array<Room, DUNGEON_ROOM_COUNT>& rooms) const = 0;
    virtual Pointer<Room> getWallNeighbor(Array<Room, DUNGEON_ROOM_COUNT>& rooms, const Room& src, const Cardinal) const = 0;
};
