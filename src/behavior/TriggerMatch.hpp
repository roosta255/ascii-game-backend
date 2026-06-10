#pragma once

#include <variant>

// Conditions evaluated before a trigger fires.
// Variants that "scan" the room also populate a scanned*Id local
// so that a subsequent TriggerEffectSet* effect can use the found entity.

// Always passes — use std::monostate for an empty (unconditional) match slot.

// Checks actor.location.roomId == memory.targetRoomId.
struct TriggerMatchReachedTargetRoom {};

// Scans room for a character with TRAIT_PICKPOCKETABLE (and not TRAIT_ACTION_READY).
// Populates scannedCharacterId on success.
struct TriggerMatchPickpocketableCharacterVisible {};

// Checks that memory.targetCharacterId is currently in the actor's room.
struct TriggerMatchTargetCharacterVisible {};

// Checks that memory.targetCharacterId is NOT in the actor's room.
struct TriggerMatchTargetCharacterLost {};

// Scans room for an unlocked chest. Populates scannedObjectId (containerCharacterId) on success.
struct TriggerMatchUnlockedChestVisible {};

// Checks that the chest identified by memory.targetObjectId is in the actor's room.
struct TriggerMatchTargetChestVisible {};

using TriggerMatch = std::variant<
    std::monostate,                        // empty slot — terminates match array scan
    TriggerMatchReachedTargetRoom,
    TriggerMatchPickpocketableCharacterVisible,
    TriggerMatchTargetCharacterVisible,
    TriggerMatchTargetCharacterLost,
    TriggerMatchUnlockedChestVisible,
    TriggerMatchTargetChestVisible
>;
