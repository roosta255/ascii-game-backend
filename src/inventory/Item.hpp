#pragma once

#include <functional>
#include <string>

#include "Array.hpp"
#include "ItemEnum.hpp"
#include "Keyframe.hpp"

class ItemFlyweight;

struct Item {
    static constexpr int MAX_KEYFRAMES = 2;

    ItemEnum type = ITEM_NIL;
    int stacks = 0;
    Array<Keyframe, MAX_KEYFRAMES> keyframes;

    bool accessFlyweight(std::function<void(const ItemFlyweight&)> consumer)const;
    bool operator==(const Item& rhs)const;
};

std::ostream& operator<<(std::ostream& os, const Item& rhs);
