#pragma once

#include "Array.hpp"
#include "Cardinal.hpp"
#include "DUNGEON_ROOM_COUNT.hpp"
#include <functional>
#include "int4.hpp"
#include <unordered_map>
#include "Pointer.hpp"
#include "Rack.hpp"
#include "Room.hpp"

class iLayout;

class GeneratorUtility {
private:
    int4 size{1,1,1,1};
    const iLayout* layout;
    std::unordered_map<int4, Pointer<Room>> mapping;
    Array<Room, DUNGEON_ROOM_COUNT>& rooms;

public:
    GeneratorUtility(Array<Room, DUNGEON_ROOM_COUNT>& rooms, const iLayout& layout);

    Pointer<Room> getRoom(const int4&);
    bool accessRoomWall(const int4&, const Cardinal, std::function<void(Room&, Wall&, Wall&, Room&)> consumer);
    bool setupTogglerOrange(const int4&, const Cardinal);
    bool setupTogglerBlue(const int4&, const Cardinal);
    bool setupDoorway(const int4&, const Cardinal);
    bool setupKeeper(const int4&, const Cardinal, const bool);
    bool setupShifter(const int4&, const Cardinal, const bool);
    bool setupHorizontalWalls (std::initializer_list<int> row, int y, int z);
    bool setupVerticalWalls (std::initializer_list<int> row, int y, int z);
};
