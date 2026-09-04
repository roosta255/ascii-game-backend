#pragma once

#include "Cardinal.hpp"
#include "Character.hpp"
#include "CharacterRelation.hpp"
#include "Chest.hpp"
#include "ConductEnum.hpp"
#include "DamageTypeBits.hpp"
#include "Item.hpp"
#include "Match.hpp"
#include "Maybe.hpp"
#include "Pointer.hpp"
#include "RequestContext.hpp"
#include "TargetEntity.hpp"

struct ActivationContext {
    Codeset& codeset;
    Pointer<RequestContext> request;
    Room& room;

    Character& character;

    // Set by the NPC dispatch loop to identify which conduct slot is active.
    // TriggerEffectSetBehavior writes state back to this slot.
    // CONDUCT_NIL when the trigger fires outside the NPC conduct loop.
    ConductEnum conductSlot = CONDUCT_NIL;

    // Set when this trigger fires from an asObserver slot; identifies the NPC who is observing.
    // -1 when the trigger fires as actor, target, or outside the NPC event loop.
    int observerCharacterId = -1;

    Pointer<Item> sourceItem;
    Maybe<int> targetItemIndex;  // absolute index into dungeon.items[], for actions like LOOT_CHEST

    // Scratch slot for handing dynamically-resolved data between the steps of a single
    // activation — e.g. a decider effect (ActivatorFindInventoryItemByTraits) resolves
    // which item is eligible, and a later effect (ActivatorTransferItem) consumes it.
    // Not persisted beyond the activation that set it.
    Maybe<ItemEnum> resolvedItem;

    TargetEntity targetEntity;

    Maybe<Cardinal> direction;

    DamageTypeBits damageTypes;

    bool isSortingState = false;
    Timestamp time;

    // Helpers to extract typed targets from targetEntity.
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

    Pointer<Chest> targetChest() const {
        Pointer<Chest> result;
        targetCharacter().access([&](Character& target) {
            request.access([&](RequestContext& req) {
                CodeEnum dummy = CODE_LOOT_CHEST_NOT_FOUND;
                result = req.match.dungeon.findChestByContainerId(target.characterId, dummy);
            });
        });
        return result;
    }

    Pointer<Chest> sourceChest() const {
        Pointer<Chest> result;
        request.access([&](RequestContext& req) {
            CodeEnum dummy = CODE_LOOT_CHEST_NOT_FOUND;
            result = req.match.dungeon.findChestByContainerId(character.characterId, dummy);
        });
        return result;
    }

    // Resolves a character starting from `anchor` (actor/target) and moving to the
    // related character described by `relation`. Self returns the anchor as-is;
    // Primary/Secondary/Tertiary look up the complementary character via
    // RoleFlyweight::findByTraits(), searching the anchor's chest if it is
    // chest-contained, otherwise its room. See ActivationContext.cpp.
    Pointer<Character> resolveCharacter(CharacterAnchor anchor, CharacterRelation relation) const;
};
