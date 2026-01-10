#include "ActivatorInactiveItem.hpp"
#include "Array.hpp"
#include "ItemEnum.hpp"
#include "ItemFlyweight.hpp"
#include "iActivator.hpp"

const Array<ItemFlyweight, ITEM_COUNT>& ItemFlyweight::getFlyweights()
{
    static auto flyweights = [](){
        Array<ItemFlyweight, ITEM_COUNT> flyweights;

        #define ITEM_DECL( name_, stacks_, droppable_, deathdrops_, activator_ ) \
            static activator_ GLOBAL_##name_##activator_; \
            flyweights.getPointer( ITEM_##name_ ).access([&](ItemFlyweight& flyweight){ \
                flyweight.stacks = stacks_; \
                flyweight.isDroppable = droppable_; \
                flyweight.isDroppedOnDeath = deathdrops_; \
                flyweight.name = #name_; \
                flyweight.isActionable = !std::is_same_v<activator_, iActivator>; \
                flyweight.useActivator = GLOBAL_##name_##activator_; \
            });
        #include "Item.enum"
        #undef ITEM_DECL

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
