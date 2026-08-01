#pragma once

#include "adl_serializer.hpp"
#include "Chest.hpp"
#include "CodeEnum.hpp"
#include "Dungeon.hpp"
#include "InventoryApiView.hpp"
#include "KeyframeView.hpp"
#include "LockFlyweight.hpp"
#include "RoleFlyweight.hpp"
#include <nlohmann/json.hpp>
#include <string>

struct ChestApiView
{
    InventoryApiView inventory;
    std::string lock;
    bool isLocked         = false;
    int containerCharacterId = -1;
    Array<KeyframeView, Chest::MAX_KEYFRAMES> keyframes;
    bool isLockActionable = false;

    inline ChestApiView() = default;

    inline ChestApiView(const Chest& model, const Dungeon& dungeon)
        : containerCharacterId(model.containerCharacterId)
        , keyframes(model.keyframes.transform([&](const Keyframe& keyframe){return KeyframeView(keyframe);}))
    {
        LockFlyweight::getFlyweights().accessConst(model.lock, [&](const LockFlyweight& flyweight) {
            this->lock     = flyweight.name;
            this->isLocked = flyweight.isLocked;
            this->isLockActionable = flyweight.isLockActionable;
        });
        if (model.containerCharacterId >= 0) {
            for (const auto& ch : dungeon.characters) {
                if (ch.characterId != model.containerCharacterId) continue;
                auto& mutableDungeon = const_cast<Dungeon&>(dungeon);
                auto inv = ch.getInventory(mutableDungeon);
                int invSize = 0;
                CodeEnum roleError = CODE_UNKNOWN_ERROR;
                ch.accessRole(roleError, [&](const RoleFlyweight& fw) { invSize = fw.inventorySize; });
                this->inventory = InventoryApiView(inv, ch.itemStartIndex < 0 ? 0 : ch.itemStartIndex, invSize);
                break;
            }
        }
    }
};

// Reflection-based JSON serialization
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ChestApiView, inventory, lock, isLocked, containerCharacterId, keyframes, isLockActionable)
