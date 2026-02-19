#pragma once

#include <ostream>

#define DOOR_DECL( name, blocks, doorActivator, lockActivator, doorway_, is_shared_doorway_ ) DOOR_##name,
enum DoorEnum
{
#include "Door.enum"
DOOR_COUNT
};
#undef DOOR_DECL

std::ostream& operator<<(std::ostream& os, const DoorEnum& code);
