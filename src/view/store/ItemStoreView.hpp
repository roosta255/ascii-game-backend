#pragma once

#include "adl_serializer.hpp"
#include "Item.hpp"
#include "ItemFlyweight.hpp"
#include "KeyframeView.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

struct ItemStoreView
{
    std::string type = "UNPARSED_ITEM";
    int stacks = 0;
    Array<KeyframeView, Item::MAX_KEYFRAMES> keyframes;

    inline ItemStoreView() = default;

    inline ItemStoreView(const Item& model): type(item_to_text(model.type)), stacks(model.stacks)
    , keyframes(model.keyframes.convert<KeyframeView>())
    {
    }

    inline operator Item() const
    {
        Item model{
            .stacks = this->stacks
        };

        ItemFlyweight::indexByString(this->type, model.type);
        model.keyframes = this->keyframes.convert<Keyframe>();
        return model;
    }
};

inline void to_json(nlohmann::json& j, const ItemStoreView& v) {
    j = {
        {"type", v.type}, {"stacks", v.stacks}, {"keyframes", v.keyframes}
    };
}

inline void from_json(const nlohmann::json& j, ItemStoreView& v) {
    j.at("type").get_to(v.type);
    j.at("stacks").get_to(v.stacks);
    if (j.contains("keyframes")) j.at("keyframes").get_to(v.keyframes);
}
