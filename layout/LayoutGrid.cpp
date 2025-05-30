#include "LayoutGrid.hpp"
#include "to_int4.hpp"
// #include <drogon/drogon.h>

Room& LayoutGrid::getEntrance (Array<Room, DUNGEON_ROOM_COUNT>& rooms) const {
    static Room fake;
    return rooms.getOrDefault(0, fake);
}

Pointer<Room> LayoutGrid::getWallNeighbor (Array<Room, DUNGEON_ROOM_COUNT>& rooms, const Room& src, const Cardinal dir) const {
    // LOG_DEBUG << "LayoutGrid{<" << width << "," << height << "," << depth << "," << times << ">}::getWallNeighbor(rooms[" << &rooms.head() << "], (" << (&src - &rooms.head()) << "), src[" << &src << "]) called";
    int offset;
    if(!rooms.containsAddress(src, offset)) {
        // LOG_DEBUG << "LayoutGrid{<" << width << "," << height << "," << depth << "," << times << ">}::getWallNeighbor(rooms[" << &rooms.head() << "], (" << (&src - &rooms.head()) << "), src[" << &src << "]) doesnt contain addr";
        return Pointer<Room>::empty();
    }
    const auto coords = getCoordinates(offset);
    const auto neighbor = to_int4(dir.getRectOffset()) + coords;
    if(!contains(neighbor)) {
        // LOG_DEBUG << "LayoutGrid{<" << width << "," << height << "," << depth << "," << times << ">}::getWallNeighbor(rooms[" << &rooms.head() << "], (" << (&src - &rooms.head()) << "), src[" << &src << "], offset: " << offset << ", coords<" << coords[0] << "," << coords[1] << "," << coords[2] << "," << coords[3] << ">, neighbor <" << neighbor[0] << "," << neighbor[1] << "," << neighbor[2] << "," << neighbor[3] << ">) isnt in the grid";
        return Pointer<Room>::empty();
    }
    // LOG_DEBUG << "LayoutGrid{<" << width << "," << height << "," << depth << "," << times << ">}::getWallNeighbor(rooms[" << &rooms.head() << "], (" << (&src - &rooms.head()) << "), src[" << &src << "], offset: " << offset << ", coords<" << coords[0] << "," << coords[1] << "," << coords[2] << "," << coords[3] << ">, neighbor <" << neighbor[0] << "," << neighbor[1] << "," << neighbor[2] << "," << neighbor[3] << ">) is in the grid";
    return rooms.getPointer(getIndex(neighbor));
}

void LayoutGrid::setDimensions (int x, int y, int z, int t) {
    width = x;
    height = y;
    depth = z;
    times = t;
}
