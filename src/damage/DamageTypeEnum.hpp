#pragma once

enum DamageTypeEnum {
#define DAMAGE_TYPE_DECL(name_, attrs_, activator_) DAMAGE_TYPE_##name_,
#define DAMAGE_TYPE_RESIST_DECL(trait_)
#define DAMAGE_TYPE_WEAKNESS_DECL(trait_)
#include "DamageType.enum"
#undef DAMAGE_TYPE_DECL
#undef DAMAGE_TYPE_RESIST_DECL
#undef DAMAGE_TYPE_WEAKNESS_DECL
    DAMAGE_TYPE_COUNT
};
