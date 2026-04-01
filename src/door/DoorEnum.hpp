#pragma once

#include <ostream>

#define DOOR_DECL( name, doorAttributes_, doorActivator, lockActivator ) DOOR_##name,
#define DOOR_LOCK_WRAPPER(...)
#define DOOR_DOOR_WRAPPER(...)
enum DoorEnum
{
#include "Door.enum"
DOOR_COUNT
};
#undef DOOR_DECL
#undef DOOR_LOCK_WRAPPER
#undef DOOR_DOOR_WRAPPER

std::ostream& operator<<(std::ostream& os, const DoorEnum& code);
