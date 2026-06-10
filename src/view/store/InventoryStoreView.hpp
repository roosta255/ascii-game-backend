#pragma once

#include "adl_serializer.hpp"
#include "Inventory.hpp"
#include "Item.hpp"
#include "ItemStoreView.hpp"
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

struct InventoryStoreView
{
    std::vector<ItemStoreView> items;

    inline InventoryStoreView() = default;

    inline InventoryStoreView(const Inventory& model) {
        for (int i = 0; i < model.size; i++) {
            items.push_back(ItemStoreView(model.items[i]));
        }
    }
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(InventoryStoreView, items)
