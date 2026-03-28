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
#define ROLE_DECL( name_, health_, moves_, actions_, feats_, activator_, traits_sourced_ ) ROLE_##name_,
#define ROLE_BITE_MODIFIER_DECL( bite_trait_modifier_ )
#define ROLE_DAMAGE_TYPES_DECL( damage_types_ )
#define ROLE_DEATH_DECL( activator_ )
#define ROLE_DAMAGE_DECL( activator_ )
#define ROLE_ATTACK_DECL( activator_ )
#define ROLE_USE_WRAPPER(...)
enum RoleEnum
{
#include "Role.enum"
ROLE_COUNT
};
#undef ROLE_DECL
#undef ROLE_BITE_MODIFIER_DECL
#undef ROLE_DAMAGE_TYPES_DECL
#undef ROLE_DEATH_DECL
#undef ROLE_DAMAGE_DECL
#undef ROLE_ATTACK_DECL
#undef ROLE_USE_WRAPPER
