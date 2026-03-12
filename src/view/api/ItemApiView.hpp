#pragma once

#include "adl_serializer.hpp"
#include "Item.hpp"
#include "ItemFlyweight.hpp"
#include "KeyframeView.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

struct ItemApiView
{
    std::string type = "UNPARSED_ITEM";
    int stacks = 0;
    int index = 0;
    bool isActionable = false;
    Array<KeyframeView, Item::MAX_KEYFRAMES> keyframes;

    inline ItemApiView() = default;

    inline ItemApiView(const Item& model): type(item_to_text(model.type)), stacks(model.stacks)
    , keyframes(model.keyframes.convert<KeyframeView>())
    {
        model.accessFlyweight([&](const ItemFlyweight& flyweight){
            this->isActionable = flyweight.isActionable;
        });
    }
};

// Reflection-based JSON serialization
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ItemApiView, type, stacks, index, isActionable, keyframes)
