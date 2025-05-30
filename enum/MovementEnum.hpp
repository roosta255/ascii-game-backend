#pragma once

#define MOVEMENT_DECL( name ) MOVEMENT_##name,
enum MovementEnum
{
#include "Movement.enum"
MOVEMENT_COUNT
};
#undef MOVEMENT_DECL

const char* movement_to_text(int);