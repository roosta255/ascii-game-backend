#pragma once

// figured out a way to have bigger rexpain canvases.
// now rooms can be rendered with one texture and call.
#define ROOM_DECL( name, width, height, isBlockingDoor, isBlockingLock ) ROOM_##name,
enum RoomEnum
{
#include "Room.enum"
ROOM_COUNT
};
#undef ROOM_DECL
