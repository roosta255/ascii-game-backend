#pragma once

#include <string>

#define ITEM_DECL( name, item_stack_size, droppable, deathdrops, activator ) ITEM_##name,
enum ItemEnum
{
#include "Item.enum"
ITEM_COUNT
};
#undef ITEM_DECL

const char* item_to_text(int);

inline std::ostream& operator<<(std::ostream& os, ItemEnum code) {
    return os << std::string(item_to_text(code));
}
