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

Location Location::makeShaftBottom(int roomId, ChannelEnum channel, Cardinal dir){
    return Location{
        .type = LOCATION_SHAFT_BOTTOM,
        .roomId = roomId,
        .channel = channel,
        .data = dir.getIndex()
    };
}

Location Location::makeShaftTop(int roomId, ChannelEnum channel, Cardinal dir){
    return Location{
        .type = LOCATION_SHAFT_TOP,
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

bool Location::accessRoomIds(const Array<Room, DUNGEON_ROOM_COUNT>& rooms, std::function<void(const int&)> consumer) const {
    consumer(roomId);
    Cardinal dir(data);
    return rooms.accessConst(roomId, [&](const Room& room){
        switch(type) {
            case LOCATION_DOOR_SHARED:
                consumer(room.getWall(dir).adjacent);
                break;
            case LOCATION_SHAFT_BOTTOM:
                consumer(room.above);
                break;
            case LOCATION_SHAFT_TOP:
                consumer(room.below);
                break;
        }
    });
}


void Location::apply(int offset, const Array<Room, DUNGEON_ROOM_COUNT>& rooms, Map<int, Map<int2, int> >& floors, Map<int, Map<int2, int> >& doors
) const {
    const auto setDoor = [&](int setDoorRoomId, Cardinal dir){
        const int2 key{channel, dir.getIndex()};
        if (!doors.access(setDoorRoomId, [&](Map<int2, int>& mapping){ mapping.set(key, offset); })) {
            doors.set(setDoorRoomId, Map<int2, int>(key, offset));
        }
    };
    Cardinal dir(data);
    const int2 key{channel, data};
    switch(type){
        case LOCATION_FLOOR:
            if (!floors.access(roomId, [&](Map<int2, int>& mapping){ mapping.set(key, offset); })) {
                floors.set(roomId, Map<int2, int>(key, offset));
            }
            return;
        case LOCATION_DOOR:
            setDoor(roomId, dir);
            return;
        case LOCATION_SHAFT_BOTTOM:
            setDoor(roomId, dir);
            rooms.accessConst(roomId, [&](const Room& room){
                setDoor(room.above, dir);
            });
            return;
        case LOCATION_SHAFT_TOP:
            setDoor(roomId, dir);
            rooms.accessConst(roomId, [&](const Room& room){
                setDoor(room.below, dir);
            });
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
