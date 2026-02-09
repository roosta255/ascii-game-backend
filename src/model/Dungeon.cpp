#include "Dungeon.hpp"
#include "iLayout.hpp"
#include "int2.hpp"
#include "int4_to_json.hpp"
#include "JsonParameters.hpp"
#include "LayoutFlyweight.hpp"
#include "DoorEnum.hpp"
#include "RoleEnum.hpp"
#include "CodeEnum.hpp"
#include <json/json.h>

bool Dungeon::accessCeilingNeighbor(const Room& source, std::function<void(const Room&)> consumer) const {
    return this->rooms.accessConst(source.above, [&](const Room& neighbor) {
        consumer(neighbor);
    });
}

bool Dungeon::accessFloorNeighbor(const Room& source, std::function<void(const Room&)> consumer) const {
    return this->rooms.accessConst(source.below, [&](const Room& neighbor) {
        consumer(neighbor);
    });
}

bool Dungeon::accessLayoutFlyweight(CodeEnum& error, std::function<void(const LayoutFlyweight&)> consumer) const
{
    const bool isAccessed = LayoutFlyweight::getFlyweights().accessConst(layout, [&](const LayoutFlyweight& flyweight){
        consumer(flyweight);
    });
    if (!isAccessed) {
        error = CODE_INACCESSIBLE_LAYOUT;
    }
    return isAccessed;
}

bool Dungeon::accessLayout(CodeEnum& error, std::function<void(const iLayout&)> consumer) const
{
    bool isAccessed = false;
    accessLayoutFlyweight(error, [&](const LayoutFlyweight& flyweight){
        const bool isLayoutAccessed = flyweight.layout.accessConst([&](const iLayout& layoutIntf){
            isAccessed = true;
            consumer(layoutIntf);
        });
        if (!isLayoutAccessed) {
            error = CODE_INACCESSIBLE_LAYOUT;
        }
    });
    return isAccessed;
}

bool Dungeon::accessWallNeighbor(Room& source, Cardinal dir, std::function<void(Wall&, Room&, int)> consumer) {
    const auto neighborId = source.getWall(dir).adjacent;
    return this->rooms.access(neighborId, [&](Room& neighbor) {
        consumer(neighbor.getWall(dir.getFlip()), neighbor, neighborId);
    });
}

bool Dungeon::accessWallNeighbor(const Room& source, Cardinal dir, std::function<void(const Wall&, const Room&, int)> consumer) const {
    const auto neighborId = source.getWall(dir).adjacent;
    return this->rooms.accessConst(neighborId, [&](const Room& neighbor) {
        consumer(neighbor.getWall(dir.getFlip()), neighbor, neighborId);
    });
}

void Dungeon::toggleDoors() {
    isBlueOpen = !isBlueOpen;
    
    // Iterate through all rooms and their walls
    for (Room& room : rooms) {
        for (Cardinal dir : Cardinal::getAllCardinals()) {
            Wall& wall = room.getWall(dir);
            
            // Update door types based on isBlueOpen
            if (wall.door == DOOR_TOGGLER_BLUE_OPEN || wall.door == DOOR_TOGGLER_BLUE_CLOSED) {
                wall.door = isBlueOpen ? DOOR_TOGGLER_BLUE_OPEN : DOOR_TOGGLER_BLUE_CLOSED;
            }
            else if (wall.door == DOOR_TOGGLER_ORANGE_OPEN || wall.door == DOOR_TOGGLER_ORANGE_CLOSED) {
                wall.door = isBlueOpen ? DOOR_TOGGLER_ORANGE_CLOSED : DOOR_TOGGLER_ORANGE_OPEN;
            }
        }
    }
}

bool Dungeon::containsRoom(const Room& room, int& roomId) const {
    // Get base address of rooms array
    auto base = reinterpret_cast<const char*>(&rooms.head());
    
    // Get address of the source room
    auto ptr = reinterpret_cast<const char*>(&room);
    
    // Verify that the room lies within the rooms array
    auto end = base + (sizeof(rooms));
    if (ptr >= base && ptr < end) {
        // Calculate room index
        roomId = static_cast<int>((ptr - base) / sizeof(Room));
        return true;
    }
    return false;
}

Pointer<Room> Dungeon::getRoom(int roomId, CodeEnum& error) {
    if (roomId < 0 || roomId >= DUNGEON_ROOM_COUNT) {
        error = CODE_ROOM_ID_OUT_OF_BOUNDS;
        return Pointer<Room>::empty();
    }
    return rooms.getPointer(roomId);
}

Pointer<const Room> Dungeon::getRoom(int roomId, CodeEnum& error) const {
    if (roomId < 0 || roomId >= DUNGEON_ROOM_COUNT) {
        error = CODE_ROOM_ID_OUT_OF_BOUNDS;
        return Pointer<const Room>::empty();
    }
    return rooms.getPointer(roomId);
}

// operators
bool Dungeon::operator==(const Dungeon& other) const {
    // Since unique_representations is true, if the bits match, the objects match.
    return std::memcmp(this, &other, sizeof(Dungeon)) == 0;
}

std::ostream& operator<<(std::ostream& os, const Dungeon& rhs) {
    return os;
}
