#pragma once

#include <string>

#include "Array.hpp"
#include "Pointer.hpp"
#include "ItemEnum.hpp"

class iActivator;

struct ItemFlyweight {
    std::string name;
    int stacks = 1;
    bool isDroppable = true;
    bool isDroppedOnDeath = true;
    bool isActionable = false;

    Pointer<const iActivator> activator;

    static const Array<ItemFlyweight, ITEM_COUNT>& getFlyweights();
    static bool indexByString(const std::string& name, ItemEnum& output);
};
