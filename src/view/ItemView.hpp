#pragma once

#include "adl_serializer.hpp"
#include "Item.hpp"
#include "ItemFlyweight.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

struct ItemView
{
    std::string type = "UNPARSED_ITEM";
    int stacks = 0;

    inline ItemView() = default;

    inline ItemView(const Item& model): type(item_to_text(model.type)), stacks()
    {
    }

    inline operator Item() const 
    {
        Item model{
            .stacks = this->stacks
        };

        ItemFlyweight::indexByString(this->type, model.type);
        return model;
    }
};

// Reflection-based JSON serialization
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ItemView, type, stacks)
