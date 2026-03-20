#pragma once

#include <string>

#define ITEM_DECL( name_, stacks_, activator_, traits_ ) ITEM_##name_,
#define ITEM_CARRY_MODIFIER_DECL( modifier_ )
#define ITEM_DAMAGE_TYPES_DECL( damage_types_ )
enum ItemEnum: int32_t
{
#include "Item.enum"
ITEM_COUNT
};
#undef ITEM_DECL
#undef ITEM_CARRY_MODIFIER_DECL
#undef ITEM_DAMAGE_TYPES_DECL

const char* item_to_text(int);

inline std::ostream& operator<<(std::ostream& os, ItemEnum code) {
    return os << std::string(item_to_text(code));
}
