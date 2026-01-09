#pragma once

#include "adl_serializer.hpp"
#include "Array.hpp"
#include "Inventory.hpp"
#include "Item.hpp"
#include "ItemStoreView.hpp"
#include <string>
#include <nlohmann/json.hpp>

struct InventoryStoreView
{
    Array<ItemStoreView, Inventory::STANDARD_ITEM_SLOTS> items;
    InventoryDigest digest;

    inline InventoryStoreView() = default;

    inline InventoryStoreView(const Inventory& model):
        items(model.items.convert<ItemStoreView>()),
        digest(model.makeDigest())
    {
    }

    inline operator Inventory() const 
    {
        return Inventory{
            .items = this->items.convert<Item>()
            // digest not needed
        };
    }
};

// Reflection-based JSON serialization
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(InventoryStoreView, items)
