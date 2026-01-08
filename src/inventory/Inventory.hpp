#pragma once

#include <functional>
#include <string>

#include "Array.hpp"
#include "CodeEnum.hpp"
#include "InventoryDigest.hpp"
#include "Item.hpp"
#include "ItemEnum.hpp"

struct Inventory {
    static constexpr int STANDARD_ITEM_SLOTS = 10;
    Array<Item, STANDARD_ITEM_SLOTS> items;

    bool accessItem(ItemEnum target, std::function<void(Item&)>);
    bool accessItem(ItemEnum target, std::function<void(const Item&)>)const;
    bool giveItem(ItemEnum type, CodeEnum& result, const bool isDryRun = false);
    bool takeItem(ItemEnum type, CodeEnum& result, const bool isDryRun = false);

    bool processDelta(ItemEnum type, const bool delta, CodeEnum& result, const bool isDryRun = false);

    InventoryDigest makeDigest() const;
};
