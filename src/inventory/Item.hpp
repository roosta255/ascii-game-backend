#pragma once

#include <functional>
#include <string>

#include "ItemEnum.hpp"

class ItemFlyweight;

struct Item {
    ItemEnum type;
    int stacks;

    bool accessFlyweight(std::function<void(const ItemFlyweight&)> consumer)const;
};