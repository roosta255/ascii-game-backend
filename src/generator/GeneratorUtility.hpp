#pragma once

#include "Array.hpp"
#include "Cardinal.hpp"
#include "DoorEnum.hpp"
#include "DUNGEON_ROOM_COUNT.hpp"
#include <functional>
#include "int4.hpp"
#include <unordered_map>
#include "Pointer.hpp"
#include "Rack.hpp"
#include "Room.hpp"
#include "RoomEnum.hpp"

class Codeset;
class iLayout;

class GeneratorUtility {
private:
    int4 size{1,1,1,1};
    const iLayout* layout;
    std::unordered_map<int4, Pointer<Room>> mapping;
    Array<Room, DUNGEON_ROOM_COUNT>& rooms;
    Codeset& codeset;

public:
    GeneratorUtility(Array<Room, DUNGEON_ROOM_COUNT>& rooms, const iLayout& layout, Codeset& codeset);

    Pointer<Room> getRoom(const int4&);
    bool accessRoomWall(const int4&, const Cardinal, std::function<void(Room&, Wall&, Wall&, Room&)> consumer);
    
    bool setupLadderUp(const int4&, const Cardinal);
    bool setupPoleUp(const int4&, const Cardinal);
    
    bool setupTogglerOrange(const int4&, const Cardinal);
    bool setupTogglerBlue(const int4&, const Cardinal);
    bool setupDoorway(const int4&, const Cardinal);
    bool setupKeeper(const int4&, const Cardinal, const bool);
    bool setupJailer(const int4&, const Cardinal, const bool);
    bool setupShifter(const int4&, const Cardinal, const bool);
    bool setupHorizontalWalls (std::initializer_list<int> row, int y, int z);
    bool setupVerticalWalls (std::initializer_list<int> row, int y, int z);
    bool setup4x1Room(const int4&);
    bool setup2x5Room(const int4&);
    bool setup3x3Room(const int4&);
    bool setupLightningRodRoom(const int4&, const bool isCubed, const bool isAwakened);
    bool setupTimeGateRoomToFuture(const int4&, const bool isCubed, const bool isAwakened);
    bool setupAdjacencyPointers();
    bool setupPowerGeneratorRoom(const int4&);

    bool setRoom(const int4&, const RoomEnum&);
    bool setDoor(const int4&, const Cardinal, const DoorEnum&);
    bool setSharedDoor(const int4&, const Cardinal, const DoorEnum& door1, const DoorEnum& door2);
};
