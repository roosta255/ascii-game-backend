#include "int4.hpp"
#include "LayoutGrid.hpp"
#include "to_int4.hpp"
// #include <drogon/drogon.h>

Room& LayoutGrid::getEntrance (Array<Room, DUNGEON_ROOM_COUNT>& rooms) const {
    static Room fake;
    return rooms.getOrDefault(0, fake);
}

bool LayoutGrid::getDelta(const Array<Room, DUNGEON_ROOM_COUNT>& rooms, const Room& src, const int4& delta, int& output) const
{
    int sourceId = -1;
    if(!rooms.containsAddress(src, sourceId)) {
        return false;
    }
    const auto coords = getCoordinates(sourceId);
    const auto neighbor = delta + coords;
    if(!contains(neighbor)) {
        return false;
    }
    output = getIndex(neighbor);
    return true;
}

bool LayoutGrid::getWallNeighbor(const Array<Room, DUNGEON_ROOM_COUNT>& rooms, const Room& src, const Cardinal dir, int& output) const
{
    return getDelta(rooms, src, to_int4(dir.getRectOffset()), output);
}

bool LayoutGrid::getTimeDelta(const Array<Room, DUNGEON_ROOM_COUNT>& rooms, const Room& src, int delta, int& output) const
{
    return getDelta(rooms, src, int4{0,0,0,delta}, output);
}

bool LayoutGrid::getDepthDelta(const Array<Room, DUNGEON_ROOM_COUNT>& rooms, const Room& src, int delta, int& output) const
{
    return getDelta(rooms, src, int4{0,0,delta,0}, output);
}

void LayoutGrid::setDimensions (int x, int y, int z, int t) {
    width = x;
    height = y;
    depth = z;
    times = t;
}
