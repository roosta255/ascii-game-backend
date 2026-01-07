#include "ItemEnum.hpp"

const char* item_to_text(int index) {
    static const char* names[] = {
        #define ITEM_DECL( name_text, item_stack_size, droppable, deathdrops, activator ) #name_text,
        #include "Item.enum"
        #undef ITEM_DECL
    };
    if (index < 0) {
        return "ITEM_INDEX_NEGATIVE";
    }
    if (index >= ITEM_COUNT) {
        return "ITEM_INDEX_OUT_OF_BOUNDS";
    }
    return names[index];
}
