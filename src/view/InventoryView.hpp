#pragma once

#include "adl_serializer.hpp"
#include "Array.hpp"
#include "Inventory.hpp"
#include "Item.hpp"
#include "ItemView.hpp"
#include <string>
#include <nlohmann/json.hpp>

struct InventoryView
{
    Array<ItemView, Inventory::STANDARD_ITEM_SLOTS> items;
    InventoryDigest digest;

    inline InventoryView() = default;

    inline InventoryView(const Inventory& model):
        items(model.items.convert<ItemView>()),
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
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(InventoryView, items)
