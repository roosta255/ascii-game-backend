#pragma once

#include "adl_serializer.hpp"
#include "Array.hpp"
#include "Inventory.hpp"
#include "Item.hpp"
#include "ItemApiView.hpp"
#include <string>
#include <nlohmann/json.hpp>

struct InventoryApiView
{
    Array<ItemApiView, Inventory::STANDARD_ITEM_SLOTS> items;
    InventoryDigest digest;

    inline InventoryApiView() = default;

    inline InventoryApiView(const Inventory& model):
        items(model.items.convert<ItemApiView>()),
        digest(model.makeDigest())
    {
        int i = 0;
        for (auto& item: this->items) {
            item.index = i;
            i++;
        }
    }
};

// Reflection-based JSON serialization
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(InventoryApiView, items)
