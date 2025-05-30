#pragma once

#define DOOR_DECL( name, blocks ) DOOR_##name,
enum DoorEnum
{
#include "Door.enum"
DOOR_COUNT
};
#undef DOOR_DECL
