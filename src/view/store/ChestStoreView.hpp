#pragma once

#include "adl_serializer.hpp"
#include "Chest.hpp"
#include "InventoryStoreView.hpp"
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

    inline ChestStoreView() = default;

    inline ChestStoreView(const Chest& model)
        : inventory(model.inventory)
        , containerCharacterId(model.containerCharacterId)
        , critterCharacterId(model.critterCharacterId)
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
        };
        LockFlyweight::indexByString(this->lock, model.lock);
        return model;
    }
};

// Reflection-based JSON serialization
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ChestStoreView, inventory, lock, containerCharacterId, critterCharacterId)
