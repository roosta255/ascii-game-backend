#pragma once

enum DamageAttrEnum {
#define DAMAGE_ATTR_DECL(name_) DAMAGE_ATTR_##name_,
#include "DamageAttr.enum"
#undef DAMAGE_ATTR_DECL
    DAMAGE_ATTR_COUNT
};
