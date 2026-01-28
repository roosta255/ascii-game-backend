#include "Location.hpp"
#include "Room.hpp"
#include "Wall.hpp"

Location Location::makeFloor(int roomId, ChannelEnum channel, int floor){
    return Location{
        .type = LOCATION_FLOOR,
        .roomId = roomId,
        .channel = channel,
        .data = floor
    };
}

Location Location::makeDoor(int roomId, ChannelEnum channel, Cardinal dir){
    return Location{
        .type = LOCATION_DOOR,
        .roomId = roomId,
        .channel = channel,
        .data = dir.getIndex()
    };
}

Location Location::makeSharedDoor(int roomId, ChannelEnum channel, Cardinal dir){
    return Location{
        .type = LOCATION_DOOR_SHARED,
        .roomId = roomId,
        .channel = channel,
        .data = dir.getIndex()
    };
}

Location Location::makeNone(){
    return Location{
        .type = LOCATION_NONE,
        .roomId = 0,
        .channel = CHANNEL_CORPOREAL,
        .data = 0
    };
}

void Location::apply(int offset, const Array<Room, DUNGEON_ROOM_COUNT>& rooms, Map<int3, int>& floors, Map<int3, int>& doors
) const {
    const auto setDoor = [&](int setDoorRoomId, Cardinal dir){
        doors.set(int3{setDoorRoomId, channel, dir.getIndex()}, offset);
    };
    Cardinal dir(data);
    switch(type){
        case LOCATION_FLOOR:
            floors.set(int3{roomId, channel, data}, offset);
            return;
        case LOCATION_DOOR:
            setDoor(roomId, dir);
            return;
        case LOCATION_DOOR_SHARED:
            setDoor(roomId, dir);
            rooms.accessConst(roomId, [&](const Room& room){
                setDoor(room.getWall(dir).adjacent, dir.getFlip());
            });
            return;
    }
}

// operators
bool Location::operator==(const Location& right)const {
    return roomId == right.roomId && channel == right.channel && type == right.type && data == right.data;
}

std::ostream& operator<<(std::ostream& os, const Location& input) {
    return os << "Location{type: " << input.type << ", roomId: " << input.roomId << ", channel: " << input.channel << ", data: " << input.data << " }";
}
