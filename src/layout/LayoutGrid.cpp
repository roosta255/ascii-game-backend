#include "int4.hpp"
#include "LayoutGrid.hpp"
#include "to_int4.hpp"
// #include <drogon/drogon.h>

Room& LayoutGrid::getEntrance (Array<Room, DUNGEON_ROOM_COUNT>& rooms) const {
    static Room fake;
    return rooms.getOrDefault(0, fake);
}

bool LayoutGrid::getWallNeighbor(const Array<Room, DUNGEON_ROOM_COUNT>& rooms, const Room& src, const Cardinal dir, int& index) const
{
    // LOG_DEBUG << "LayoutGrid{<" << width << "," << height << "," << depth << "," << times << ">}::getWallNeighbor(rooms[" << &rooms.head() << "], (" << (&src - &rooms.head()) << "), src[" << &src << "]) called";
    if(!rooms.containsAddress(src, index)) {
        // LOG_DEBUG << "LayoutGrid{<" << width << "," << height << "," << depth << "," << times << ">}::getWallNeighbor(rooms[" << &rooms.head() << "], (" << (&src - &rooms.head()) << "), src[" << &src << "]) doesnt contain addr";
        return false;
    }
    const auto coords = getCoordinates(index);
    const auto neighbor = to_int4(dir.getRectOffset()) + coords;
    if(!contains(neighbor)) {
        // LOG_DEBUG << "LayoutGrid{<" << width << "," << height << "," << depth << "," << times << ">}::getWallNeighbor(rooms[" << &rooms.head() << "], (" << (&src - &rooms.head()) << "), src[" << &src << "], offset: " << offset << ", coords<" << coords[0] << "," << coords[1] << "," << coords[2] << "," << coords[3] << ">, neighbor <" << neighbor[0] << "," << neighbor[1] << "," << neighbor[2] << "," << neighbor[3] << ">) isnt in the grid";
        return false;
    }
    // LOG_DEBUG << "LayoutGrid{<" << width << "," << height << "," << depth << "," << times << ">}::getWallNeighbor(rooms[" << &rooms.head() << "], (" << (&src - &rooms.head()) << "), src[" << &src << "], offset: " << offset << ", coords<" << coords[0] << "," << coords[1] << "," << coords[2] << "," << coords[3] << ">, neighbor <" << neighbor[0] << "," << neighbor[1] << "," << neighbor[2] << "," << neighbor[3] << ">) is in the grid";
    index = getIndex(neighbor);
    return true;
}

void LayoutGrid::setDimensions (int x, int y, int z, int t) {
    width = x;
    height = y;
    depth = z;
    times = t;
}
