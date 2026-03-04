#include "ItemEnum.hpp"

const char* item_to_text(int index) {
    static const char* names[] = {
        #define ITEM_DECL( name_, stacks_, activator_, traits_ ) #name_,
        #define ITEM_CARRY_MODIFIER_DECL( modifier_ ) 
        #include "Item.enum"
        #undef ITEM_DECL
        #undef ITEM_CARRY_MODIFIER_DECL
    };
    if (index < 0) {
        return "ITEM_INDEX_NEGATIVE";
    }
    if (index >= ITEM_COUNT) {
        return "ITEM_INDEX_OUT_OF_BOUNDS";
    }
    return names[index];
}
