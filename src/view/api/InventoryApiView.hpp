#pragma once

#include "adl_serializer.hpp"
#include "Inventory.hpp"
#include "Item.hpp"
#include "ItemApiView.hpp"
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

struct InventoryApiView
{
    std::vector<ItemApiView> items;
    bool isEmpty = false;
    int keys = 0;
    int inventorySize = 0;

    inline InventoryApiView() = default;

    inline InventoryApiView(const Inventory& model, int startIndex, int inventorySize)
    : inventorySize(inventorySize) {
        for (int i = 0; i < model.size; i++) {
            ItemApiView view(model.items[i]);
            view.index = startIndex + i;
            this->items.push_back(view);
        }
        const auto digest = model.makeDigest();
        this->keys = digest.keys;
        this->isEmpty = digest.isEmpty;
    }
};

// Reflection-based JSON serialization
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(InventoryApiView, items, isEmpty, keys, inventorySize)
