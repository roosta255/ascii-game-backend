#pragma once

#include "Array.hpp"
#include "Cardinal.hpp"
#include "ChannelEnum.hpp"
#include "DUNGEON_ROOM_COUNT.hpp"
#include "int3.hpp"
#include "LocationEnum.hpp"
#include "Map.hpp"
#include <ostream>
#include "Room.hpp"
#include <unordered_map>

// channels to add: corporeal, digital, ethereal, minish, item

struct Location {
public:
    // members
    LocationEnum type = LOCATION_NONE;
    int32_t roomId = -1;
    ChannelEnum channel = CHANNEL_CORPOREAL;
    int32_t data = 0;

    // makers
    static Location makeFloor(int roomId, ChannelEnum channel, int floor);
    static Location makeDoor(int roomId, ChannelEnum channel, Cardinal dir);
    static Location makeShaftBottom(int roomId, ChannelEnum channel, Cardinal dir);
    static Location makeShaftTop(int roomId, ChannelEnum channel, Cardinal dir);
    static Location makeSharedDoor(int roomId, ChannelEnum channel, Cardinal dir);
    static Location makeNone();

    // functions
    bool accessRoomIds(const Array<Room, DUNGEON_ROOM_COUNT>& rooms, std::function<void(const int&)> consumer)const;
    void apply(
        int offset,
        const Array<Room, DUNGEON_ROOM_COUNT>& rooms,
        // TODO: add ceiling, flying, underground positions. 
        Map<int, Map<int2, int> >& floors,
        Map<int, Map<int2, int> >& doors
    ) const;

    // operators
    bool operator==(const Location& right)const;
};

std::ostream& operator<<(std::ostream& os, const Location& code);
