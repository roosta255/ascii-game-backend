#pragma once

#include <functional>

#include "CodeEnum.hpp"
#include "InventoryDigest.hpp"
#include "Item.hpp"
#include "ItemEnum.hpp"
#include "TraitBits.hpp"

// Non-owning view over a contiguous slice of Item in Dungeon::items[].
// Items are owned by Dungeon; Inventory is a temporary accessor built on-demand
// in controller/activator code and never stored inside Match data.
struct Inventory {
    static constexpr int PLAYER_ITEM_SLOTS = 10;

    Item* items = nullptr;
    int size = 0;

    Inventory() = default;
    Inventory(Item* items, int size) : items(items), size(size) {}

    bool accessItem(ItemEnum target, std::function<void(Item&)>);
    bool accessItem(ItemEnum target, std::function<void(const Item&)>) const;
    bool accessItemByTraits(TraitBits required, TraitBits restricted, std::function<void(const Item&)> consumer) const;
    bool accessItems(ItemEnum target, std::function<void(const Item&)>) const;
    bool giveItem(ItemEnum type, CodeEnum& result, const bool isDryRun = false);
    bool takeItem(ItemEnum type, CodeEnum& result, const bool isDryRun = false);

    bool processDelta(ItemEnum type, const bool delta, CodeEnum& result, const bool isDryRun = false);
    bool accessItem(int index, CodeEnum& result, std::function<void(Item&)>);

    bool isValid() const { return items != nullptr && size > 0; }

    InventoryDigest makeDigest() const;
};
