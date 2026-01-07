#pragma once

#include "adl_serializer.hpp"
#include "Array.hpp"
#include "Inventory.hpp"
#include "Item.hpp"
#include "ItemView.hpp"
#include <string>
#include <nlohmann/json.hpp>

// TODO: don't save digest to store
struct InventoryDigestView
{
    bool isEmpty = false;
    int keys = 0;

    inline InventoryDigestView() = default;

    inline InventoryDigestView(const InventoryDigest& model):
        isEmpty(model.isEmpty),
        keys(model.keys)
    {
    }

    inline operator InventoryDigest() const 
    {
        return InventoryDigest{
            .isEmpty = this->isEmpty,
            .keys = this->keys
        };
    }
};

// Reflection-based JSON serialization
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(InventoryDigestView, isEmpty, keys)
