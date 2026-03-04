#include "ActivatorInactiveItem.hpp"
#include "Array.hpp"
#include "ItemEnum.hpp"
#include "ItemFlyweight.hpp"
#include "iActivator.hpp"

const Array<ItemFlyweight, ITEM_COUNT>& ItemFlyweight::getFlyweights()
{
    static auto flyweights = [](){
        Array<ItemFlyweight, ITEM_COUNT> flyweights;
        ItemEnum lastItem = ITEM_NIL; // Tracker for the current "Parent" trait

        #define ITEM_DECL( name_, stacks_, activator_, traits_ ) \
            lastItem = ITEM_##name_; \
            static activator_ GLOBAL_##name_##activator_; \
            flyweights.getPointer( lastItem ).access([&](ItemFlyweight& flyweight){ \
                flyweight.stacks = stacks_; \
                flyweight.name = #name_; \
                flyweight.isActionable = !std::is_same_v<activator_, iActivator>; \
                flyweight.useActivator = GLOBAL_##name_##activator_; \
                flyweight.itemAttributes = makeTraitBits traits_ ; \
            });

        #define ITEM_CARRY_MODIFIER_DECL( modifier_ ) \
            flyweights.getPointer( lastTrait ).access([&](ItemFlyweight& flyweight){ \
                flyweight.carryModifiers.push_back(TraitModifier modifier_); \
            });

        #include "Item.enum"
        #undef ITEM_DECL
        #undef ITEM_CARRY_MODIFIER_DECL

        return flyweights;
    }();
    return flyweights;
}

bool ItemFlyweight::indexByString
(
    const std::string& name,
    ItemEnum& output
) {
    int count = 0;
    const auto& flyweights = getFlyweights(); 
    for(const ItemFlyweight& flyweight: flyweights) {
        if(name == flyweight.name) {
            output = (ItemEnum)count;
            return true;
        }
        count++;
    }
    return false;
}
