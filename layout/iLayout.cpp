#include "iLayout.hpp"

Pointer<Room> iLayout::getWallNeighbor(Array<Room, DUNGEON_ROOM_COUNT>& rooms, const Room& src, const Cardinal dir) const
{
    int index;
    if (!this->getWallNeighbor(rooms, src, dir, index)) {
        return Pointer<Room>::empty();
    }
    return rooms.getPointer(index);
}

Pointer<const Room> iLayout::getWallNeighbor(const Array<Room, DUNGEON_ROOM_COUNT>& rooms, const Room& src, const Cardinal dir) const
{
    int index;
    if (!this->getWallNeighbor(rooms, src, dir, index)) {
        return Pointer<const Room>::empty();
    }
    return rooms.getPointer(index);
}
