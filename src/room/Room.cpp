#include "int2.hpp"
#include "Room.hpp"
#include "RoomFlyweight.hpp"
#include "Match.hpp"
#include "JsonParameters.hpp"
#include <json/json.h>

Wall& Room::getWall (Cardinal dir) {
    // shouldn't be possible
    static Wall nil;
    Wall* result = &nil;
    walls.access(dir.getIndex(), [&](Wall& accessed){
        result = &accessed;
    });
    return *result;
}

const Wall& Room::getWall (Cardinal dir) const {
    // shouldn't be possible
    static Wall nil;
    const Wall* result = &nil;
    walls.accessConst(dir.getIndex(), [&](const Wall& accessed){
        result = &accessed;
    });
    return *result;
}

int Room::getOffset(const Array<Room, DUNGEON_ROOM_COUNT>& rooms)const {
    return this - &rooms.head();
}

int2 Room::getFloorSize() const {
    int2 size{0,0};
    RoomFlyweight::getFlyweights().accessConst(this->type, [&](const RoomFlyweight& flyweight) {
        size = int2{flyweight.width, flyweight.height};
    });
    return size;
}

void Room::iterateFloor(std::function<void(const int, const int2)> consumer) const {
    const auto floorSize = getFloorSize();
    const auto floorCount = floorSize[0] * floorSize[1];

    int i = 0;
    for (int y = 0; y < floorSize[1]; y++) {
        for (int x = 0; x < floorSize[0]; x++) {
            consumer(i++, int2{x, y});
        }
    }
}

// REMOVE: may no longer be needed
void Room::setType(const RoomEnum& input) {
    this->type = input;
}

int Room::getDeltaTime(const int delta)const {
    switch (delta) {
        case 1: return this->posterior;
        case -1: return this->anterior;
    }
    return -1;
}

// operators

std::ostream& operator<<(std::ostream& os, const Room& rhs) {
    return os << "Room{}";
}
