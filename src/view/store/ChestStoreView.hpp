#pragma once

#include "adl_serializer.hpp"
#include "Chest.hpp"
#include "InventoryStoreView.hpp"
#include "KeyframeView.hpp"
#include "LockEnum.hpp"
#include "LockFlyweight.hpp"
#include <nlohmann/json.hpp>
#include <string>

struct ChestStoreView
{
    InventoryStoreView inventory;
    std::string lock = "UNPARSED_LOCK";
    int containerCharacterId = -1;
    int critterCharacterId   = -1;
    Array<KeyframeView, Chest::MAX_KEYFRAMES> keyframes;

    inline ChestStoreView() = default;

    inline ChestStoreView(const Chest& model)
        : inventory(model.inventory)
        , containerCharacterId(model.containerCharacterId)
        , critterCharacterId(model.critterCharacterId)
        , keyframes(model.keyframes.convert<KeyframeView>())
    {
        LockFlyweight::getFlyweights().accessConst(model.lock, [&](const LockFlyweight& flyweight) {
            this->lock = flyweight.name;
        });
    }

    inline operator Chest() const
    {
        Chest model{
            .inventory             = this->inventory,
            .containerCharacterId  = this->containerCharacterId,
            .critterCharacterId    = this->critterCharacterId,
            .keyframes             = this->keyframes.convert<Keyframe>(),
        };
        LockFlyweight::indexByString(this->lock, model.lock);
        return model;
    }
};

inline void to_json(nlohmann::json& j, const ChestStoreView& v) {
    j = {
        {"inventory", v.inventory}, {"lock", v.lock},
        {"containerCharacterId", v.containerCharacterId}, {"critterCharacterId", v.critterCharacterId},
        {"keyframes", v.keyframes}
    };
}

inline void from_json(const nlohmann::json& j, ChestStoreView& v) {
    j.at("inventory").get_to(v.inventory);
    j.at("lock").get_to(v.lock);
    j.at("containerCharacterId").get_to(v.containerCharacterId);
    j.at("critterCharacterId").get_to(v.critterCharacterId);
    if (j.contains("keyframes")) j.at("keyframes").get_to(v.keyframes);
}
