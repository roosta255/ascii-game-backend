#pragma once

#include <functional>
#include <string>

#include "Array.hpp"
#include "CodeEnum.hpp"
#include "InventoryDigest.hpp"
#include "Item.hpp"
#include "ItemEnum.hpp"

class Match;

struct Inventory {
    static constexpr int STANDARD_ITEM_SLOTS = 10;
    Array<Item, STANDARD_ITEM_SLOTS> items;

    bool accessItem(ItemEnum target, std::function<void(Item&)>);
    bool accessItem(ItemEnum target, std::function<void(const Item&)>)const;
    bool pushItem(Item& src);

    InventoryDigest makeDigest() const;
};
