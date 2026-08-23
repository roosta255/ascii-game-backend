#pragma once

// Shared implementation of TriggerEffectScanCharacters used by both
// TriggerWrapper (event-driven) and Conduct (tick-driven proposal effects).

#include "ActivationContext.hpp"
#include "Array.hpp"
#include "Conduct.hpp"
#include "ConductMemory.hpp"
#include "ItemFlyweight.hpp"
#include "LockFlyweight.hpp"
#include "MatchController.hpp"
#include "RequestContext.hpp"
#include "TriggerEffect.hpp"

inline void applyInnerEffect(
    const TriggerInnerEffect& e,
    ActivationContext&        activation,
    Conduct&                  conduct,
    ConductMemory&            conductMem,
    int                       scannedCharacterId,
    int                       scannedObjectId,
    int                       scannedRoomId)
{
    std::visit([&](auto&& eff) {
        using T = std::decay_t<decltype(eff)>;
        if constexpr (std::is_same_v<T, std::monostate>) {}
        else if constexpr (std::is_same_v<T, TriggerEffectSetBehavior>)
            { conductMem.state = eff.behavior; }
        else if constexpr (std::is_same_v<T, TriggerEffectSetVar>)
            { conduct.set(eff.var, scannedCharacterId); }
        else if constexpr (std::is_same_v<T, TriggerEffectSetVarFromObj>)
            { conduct.set(eff.var, scannedObjectId); }
        else if constexpr (std::is_same_v<T, TriggerEffectSetVarFromRoom>)
            { conduct.set(eff.var, scannedRoomId); }
        else if constexpr (std::is_same_v<T, TriggerEffectSetVarFromContext>)
            { activation.targetCharacter().access([&](Character& ch) { conduct.set(eff.var, ch.characterId); }); }
        else if constexpr (std::is_same_v<T, TriggerEffectClearVar>)
            { conduct.set(eff.var, -1); }
    }, e);
}

inline bool applyScanCharacters(
    const TriggerEffectScanCharacters& eff,
    ActivationContext& activation,
    Conduct& conduct)
{
    bool found = false;
    activation.request.access([&](RequestContext& req) {
        const int actorRoomId    = activation.character.location.roomId;
        const int ignoredBits    = (eff.roomsIgnoredVar != CONDUCT_MEMORY_VARIABLE_COUNT)
                                     ? conduct.get(eff.roomsIgnoredVar) : 0;
        const int scannedBitMask = (eff.roomsScannedVar != CONDUCT_MEMORY_VARIABLE_COUNT)
                                     ? conduct.get(eff.roomsScannedVar) : -1;
        const bool hasItemFilter = eff.itemAccepted.head() != ITEM_UNALLOCATED
                                || eff.itemRejected.head() != ITEM_UNALLOCATED
                                || eff.itemTraitsRequired.isAny()
                                || eff.itemTraitsRestricted.isAny();
        const bool hasLockFilter = eff.lockTraitsRequired.isAny()
                                || eff.lockTraitsRestricted.isAny()
                                || eff.locksAccepted.head() != LOCK_COUNT
                                || eff.locksRejected.head() != LOCK_COUNT;

        int firstMatchedChar      = -1, firstMatchedRoom  = -1;
        int firstRejectedChar     = -1, firstRejectedRoom = -1;
        RoleEnum firstMatchedRoleVal  = ROLE_EMPTY;
        RoleEnum firstRejectedRoleVal = ROLE_EMPTY;
        int matchedBits  = 0;
        int scannedBits  = 0;
        int rejectedBits = 0;

        auto processChar = [&](int cid, RoleEnum role) {
            if (eff.onlyObserver && cid != activation.character.characterId) return;
            if (eff.ignoreSelf   && cid == activation.character.characterId) return;

            int charRoom = -1;
            for (int r = 0; r < DUNGEON_ROOM_COUNT && charRoom == -1; r++) {
                bool inRoom = false;
                req.controller.isCharacterWithinRoom(cid, r, inRoom);
                if (inRoom) charRoom = r;
            }
            if (charRoom == -1) return;

            const bool inActorRoom = (charRoom == actorRoomId);
            if (eff.ignoreInRoom      &&  inActorRoom) return;
            if (eff.ignoreOutsideRoom && !inActorRoom) return;

            const int roomBit = (charRoom < 32) ? (1 << charRoom) : 0;
            if (roomBit) {
                if (ignoredBits & roomBit) return;
                if (scannedBitMask != -1 && !(scannedBitMask & roomBit)) return;
            }
            scannedBits |= roomBit;

            const auto traits = req.controller.getTraitsComputed(cid).final;
            bool passes = true;
            if (eff.traitsRequired.isAny()   && (eff.traitsRequired   - traits).isAny()) passes = false;
            if (eff.traitsRestricted.isAny() && (eff.traitsRestricted & traits).isAny()) passes = false;
            if (passes && eff.rolesAccepted.head() != ROLE_COUNT) {
                bool roleMatched = false;
                for (int i = 0; i < TriggerEffectScanCharacters::MAX_ROLE_FILTER && eff.rolesAccepted.getOrDefault(i, ROLE_COUNT) != ROLE_COUNT; i++)
                    if (role == eff.rolesAccepted.getOrDefault(i, ROLE_COUNT)) { roleMatched = true; break; }
                if (!roleMatched) passes = false;
            }
            if (passes && eff.rolesRejected.head() != ROLE_COUNT) {
                for (int i = 0; i < TriggerEffectScanCharacters::MAX_ROLE_FILTER && eff.rolesRejected.getOrDefault(i, ROLE_COUNT) != ROLE_COUNT; i++)
                    if (role == eff.rolesRejected.getOrDefault(i, ROLE_COUNT)) { passes = false; break; }
            }

            if (passes && hasItemFilter) {
                bool hasMatchingItem = false;
                CodeEnum itemErr = CODE_UNSET;
                req.match.getCharacter(cid, itemErr).access([&](Character& ch) {
                    const auto inv = ch.getInventory(req.match.dungeon);
                    for (int i = 0; i < inv.size && !hasMatchingItem; i++) {
                        const Item& item = inv.items[i];
                        if (item.type == ITEM_UNALLOCATED || item.stacks == 0) continue;
                        if (eff.itemAccepted.head() != ITEM_UNALLOCATED) {
                            bool itemMatched = false;
                            for (int j = 0; j < TriggerEffectScanCharacters::MAX_ITEM_FILTER && eff.itemAccepted.getOrDefault(j, ITEM_UNALLOCATED) != ITEM_UNALLOCATED; j++)
                                if (item.type == eff.itemAccepted.getOrDefault(j, ITEM_UNALLOCATED)) { itemMatched = true; break; }
                            if (!itemMatched) continue;
                        }
                        if (eff.itemRejected.head() != ITEM_UNALLOCATED) {
                            bool blocked = false;
                            for (int j = 0; j < TriggerEffectScanCharacters::MAX_ITEM_FILTER && eff.itemRejected.getOrDefault(j, ITEM_UNALLOCATED) != ITEM_UNALLOCATED; j++)
                                if (item.type == eff.itemRejected.getOrDefault(j, ITEM_UNALLOCATED)) { blocked = true; break; }
                            if (blocked) continue;
                        }
                        if (eff.itemTraitsRequired.isAny() || eff.itemTraitsRestricted.isAny()) {
                            bool traitPass = true;
                            item.accessFlyweight([&](const ItemFlyweight& fw) {
                                if (eff.itemTraitsRequired.isAny()   && (eff.itemTraitsRequired   - fw.itemAttributes).isAny()) traitPass = false;
                                if (eff.itemTraitsRestricted.isAny() && (eff.itemTraitsRestricted & fw.itemAttributes).isAny()) traitPass = false;
                            });
                            if (!traitPass) continue;
                        }
                        hasMatchingItem = true;
                    }
                });
                if (!hasMatchingItem) passes = false;
            }

            if (passes && hasLockFilter) {
                bool lockPasses = false;
                req.controller.getChestByContainerId(cid).access([&](Chest& chest) {
                    LockFlyweight::getFlyweights().accessConst((int)chest.lock, [&](const LockFlyweight& lf) {
                        bool ok = true;
                        if (eff.lockTraitsRequired.isAny()   && (eff.lockTraitsRequired   - lf.lockAttributes).isAny()) ok = false;
                        if (eff.lockTraitsRestricted.isAny() && (eff.lockTraitsRestricted & lf.lockAttributes).isAny()) ok = false;
                        if (ok && eff.locksAccepted.head() != LOCK_COUNT) {
                            bool lockMatched = false;
                            for (int i = 0; i < TriggerEffectScanCharacters::MAX_LOCK_ACCEPTED && eff.locksAccepted.getOrDefault(i, LOCK_COUNT) != LOCK_COUNT; i++)
                                if (chest.lock == eff.locksAccepted.getOrDefault(i, LOCK_COUNT)) { lockMatched = true; break; }
                            if (!lockMatched) ok = false;
                        }
                        if (ok && eff.locksRejected.head() != LOCK_COUNT) {
                            for (int i = 0; i < TriggerEffectScanCharacters::MAX_LOCK_REJECTED && eff.locksRejected.getOrDefault(i, LOCK_COUNT) != LOCK_COUNT; i++)
                                if (chest.lock == eff.locksRejected.getOrDefault(i, LOCK_COUNT)) { ok = false; break; }
                        }
                        lockPasses = ok;
                    });
                });
                if (!lockPasses) passes = false;
            }

            if (passes) {
                if (firstMatchedChar == -1) { firstMatchedChar = cid; firstMatchedRoom = charRoom; firstMatchedRoleVal = role; }
                matchedBits |= roomBit;
            } else {
                if (firstRejectedChar == -1) { firstRejectedChar = cid; firstRejectedRoom = charRoom; firstRejectedRoleVal = role; }
                rejectedBits |= roomBit;
            }
        };

        if (eff.scanBuilders)
            for (auto& b : req.match.builders)
                processChar(b.character.characterId, b.character.role);
        if (eff.scanDungeon)
            for (auto& ch : req.match.dungeon.characters)
                processChar(ch.characterId, ch.role);

        auto writeVar = [&](ConductMemoryVariableEnum var, int val) {
            if (var != CONDUCT_MEMORY_VARIABLE_COUNT) conduct.set(var, val);
        };
        writeVar(eff.firstMatchedCharId,    firstMatchedChar);
        writeVar(eff.firstMatchedRoomId,    firstMatchedRoom);
        writeVar(eff.firstMatchedRole,      (int)firstMatchedRoleVal);
        writeVar(eff.firstRejectedCharId,   firstRejectedChar);
        writeVar(eff.firstRejectedRoomId,   firstRejectedRoom);
        writeVar(eff.firstRejectedRole,     (int)firstRejectedRoleVal);
        writeVar(eff.matchedRoomBitsVar,    matchedBits);
        writeVar(eff.unmatchedRoomBitsVar,  scannedBits & ~matchedBits);
        writeVar(eff.rejectedRoomBitsVar,   rejectedBits);
        writeVar(eff.unrejectedRoomBitsVar, scannedBits & ~rejectedBits);
        found = (firstMatchedChar != -1);
    });
    return found;
}
