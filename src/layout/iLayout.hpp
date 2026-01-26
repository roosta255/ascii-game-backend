#pragma once

#include "Array.hpp"
#include "Cardinal.hpp"
#include "DUNGEON_ROOM_COUNT.hpp"
#include "Pointer.hpp"
#include "Room.hpp"
// provides entrance
// provides neighbors

class iLayout {
public:
    virtual Room& getEntrance(Array<Room, DUNGEON_ROOM_COUNT>& rooms) const = 0;
    virtual bool getWallNeighbor(const Array<Room, DUNGEON_ROOM_COUNT>& rooms, const Room& src, const Cardinal, int& output) const = 0;
    virtual bool getTimeDelta(const Array<Room, DUNGEON_ROOM_COUNT>& rooms, const Room& src, int delta, int& output) const = 0;
    virtual bool getDepthDelta(const Array<Room, DUNGEON_ROOM_COUNT>& rooms, const Room& src, int delta, int& output) const = 0;
    Pointer<Room> getWallNeighbor(Array<Room, DUNGEON_ROOM_COUNT>& rooms, const Room& src, const Cardinal) const;
    Pointer<const Room> getWallNeighbor(const Array<Room, DUNGEON_ROOM_COUNT>& rooms, const Room& src, const Cardinal) const;
};
