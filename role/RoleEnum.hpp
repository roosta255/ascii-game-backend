#pragma once

// everything in this list exists
// because it's a part of the 
// player inventory thst other
// systems would integrate with.
// in order to remove any of these
// you'd first have to ensure the
// game can be played without it.
// but it's probably just simpler 
// to keep it.
#define ROLE_DECL( name, health, foggy, movey, moves, acty, actions, feats ) ROLE_##name,
enum RoleEnum
{
#include "Role.enum"
ROLE_COUNT
};
#undef ROLE_DECL
