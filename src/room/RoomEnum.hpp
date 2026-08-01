#pragma once

#include <ostream>
#include <string>

// figured out a way to have bigger rexpain canvases.
// now rooms can be rendered with one texture and call.
#define ROOM_DECL( name, width, height, isBlockingDoor, isBlockingLock, room_source_attributes_ ) ROOM_##name,
enum RoomEnum
{
#include "Room.enum"
ROOM_COUNT
};
#undef ROOM_DECL

const char* room_to_text(int);

inline std::ostream& operator<<(std::ostream& os, const RoomEnum& code) {
    return os << std::string(room_to_text(code));
}
