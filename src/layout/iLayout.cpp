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

bool iLayout::setupAdjacencyPointers(Array<Room, DUNGEON_ROOM_COUNT>& rooms) const {
    bool isSuccess = false;
    for (auto& room: rooms) {
        for (const auto& dir: Cardinal::getAllCardinals()) {
            isSuccess |= this->getWallNeighbor(rooms, room, dir, room.getWall(dir).adjacent);
        }
        isSuccess |= this->getDepthDelta(rooms, room, -1, room.below);
        isSuccess |= this->getDepthDelta(rooms, room, 1, room.above);
        isSuccess |= this->getTimeDelta(rooms, room, -1, room.anterior);
        isSuccess |= this->getTimeDelta(rooms, room, 1, room.posterior);
    }
    return isSuccess;
}
