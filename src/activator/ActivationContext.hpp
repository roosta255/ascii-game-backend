#pragma once

#include "Cardinal.hpp"
#include "Character.hpp"
#include "DamageTypeBits.hpp"
#include "Item.hpp"
#include "Maybe.hpp"
#include "Pointer.hpp"
#include "RequestContext.hpp"
#include "TargetEntity.hpp"

class Inventory;

struct ActivationContext {
    Codeset& codeset;
    Pointer<RequestContext> request;
    Room& room;

    Character& character;

    Pointer<Item> sourceItem;

    Pointer<Inventory> sourceInventory;
    Pointer<Inventory> targetInventory;

    TargetEntity targetEntity;

    // Item index within targetInventory — used by activators that target
    // a specific item slot inside a container (e.g. ActivatorLootChest).
    Maybe<int> targetItemSlot;

    Maybe<Cardinal> direction;

    DamageTypeBits damageTypes;

    bool isSortingState = false;
    Timestamp time;

    // Helpers to extract typed targets from targetEntity.
    // Return an empty Pointer when targetEntity holds a different type.
    Pointer<Character> targetCharacter() const {
        auto* p = std::get_if<Pointer<Character>>(&targetEntity);
        return p ? *p : Pointer<Character>{};
    }
    Pointer<Item> targetItem() const {
        auto* p = std::get_if<Pointer<Item>>(&targetEntity);
        return p ? *p : Pointer<Item>{};
    }
    Pointer<Wall> targetDoor() const {
        auto* p = std::get_if<DoorTarget>(&targetEntity);
        return p ? p->wall : Pointer<Wall>{};
    }
    Pointer<Wall> targetLock() const {
        auto* p = std::get_if<LockTarget>(&targetEntity);
        return p ? p->wall : Pointer<Wall>{};
    }
    Pointer<Wall> targetWall() const {
        if (auto* p = std::get_if<DoorTarget>(&targetEntity)) return p->wall;
        if (auto* p = std::get_if<LockTarget>(&targetEntity)) return p->wall;
        return Pointer<Wall>{};
    }
};
