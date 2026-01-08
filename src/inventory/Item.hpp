#pragma once

#include <functional>
#include <string>

#include "ItemEnum.hpp"

class ItemFlyweight;

struct Item {
    ItemEnum type = ITEM_NIL;
    int stacks = 0;

    bool accessFlyweight(std::function<void(const ItemFlyweight&)> consumer)const;
};