#pragma once

#include "adl_serializer.hpp"
#include "Chest.hpp"
#include "InventoryApiView.hpp"
#include "KeyframeView.hpp"
#include "LockFlyweight.hpp"
#include <nlohmann/json.hpp>
#include <string>

struct ChestApiView
{
    InventoryApiView inventory;
    std::string lock;
    bool isLocked         = false;
    int containerCharacterId = -1;
    int critterCharacterId   = -1;
    Array<KeyframeView, Chest::MAX_KEYFRAMES> keyframes;

    inline ChestApiView() = default;

    inline ChestApiView(const Chest& model)
        : inventory(model.inventory)
        , containerCharacterId(model.containerCharacterId)
        , critterCharacterId(model.critterCharacterId)
        , keyframes(model.keyframes.transform([&](const Keyframe& keyframe){return KeyframeView(keyframe);}))
    {
        LockFlyweight::getFlyweights().accessConst(model.lock, [&](const LockFlyweight& flyweight) {
            this->lock     = flyweight.name;
            this->isLocked = flyweight.isLocked;
        });
    }
};

// Reflection-based JSON serialization
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ChestApiView, inventory, lock, isLocked, containerCharacterId, critterCharacterId, keyframes)
