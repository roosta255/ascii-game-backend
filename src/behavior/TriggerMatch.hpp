#pragma once

#include <variant>
#include "Array.hpp"
#include "ConductMemoryVariableEnum.hpp"
#include "LockEnum.hpp"
#include "WrapperConfig.hpp"

// Conditions evaluated before a trigger fires.
// Variants that "scan" the room also populate a scanned*Id local
// so that a subsequent TriggerEffectSet* effect can use the found entity.

// Always passes — use std::monostate for an empty (unconditional) match slot.

// Checks the activation actor against match (traits/roles) and condition (required/restricted).
struct TriggerMatchActor {
    WrapperConfig::Match     match     = {};
    WrapperConfig::Condition condition = {};
};

// Checks the activation tool (source item) against match (traits/items) and condition.
struct TriggerMatchTool {
    WrapperConfig::Match     match     = {};
    WrapperConfig::Condition condition = {};
};

// Checks the activation target (character, item, or wall) against match and condition.
// Dispatches on the actual target type: characters use traits/roles, items use traits/items,
// walls use traits/doors. Returns false if no target is present.
struct TriggerMatchTarget {
    WrapperConfig::Match     match     = {};
};

// Checks actor.location.roomId == memory[var].
struct TriggerMatchReachedTargetRoom {
    ConductMemoryVariableEnum var = CONDUCT_MEMORY_ROOM_ID;
};

// Selects which ActivationContext field TriggerMatchCharacter checks.
// Actor/Target/Observer resolve to characters; scannedCharacterId is set on a match.
// Observer falls back to activation.character when observerCharacterId == -1.
// Tool resolves to activation.sourceItem: only item filters apply; scannedCharacterId is not written.
enum class TriggerMatchActivationSource : int { Actor = 0, Tool = 1, Target = 2, Observer = 3 };

// Checks a character (or item) from within the ActivationContext against rich filters.
// Actor = activation.character, Target = activation.targetCharacter(), Tool = activation.sourceItem.
// Returns false if the selected field is absent or does not satisfy all active filters.
// Sets scannedCharacterId on success (Actor/Target only).
struct TriggerMatchCharacter {
    static constexpr int MAX_ROLE_FILTER   = 8;
    static constexpr int MAX_ITEM_FILTER   = 8;
    static constexpr int MAX_LOCK_ACCEPTED = 8;
    static constexpr int MAX_LOCK_REJECTED = 6;

    TriggerMatchActivationSource source = TriggerMatchActivationSource::Target;

    // ── character filter ──────────────────────────────────────────────────────
    TraitBits traitsRequired   = {};
    TraitBits traitsRestricted = {};
    Array<RoleEnum, MAX_ROLE_FILTER> rolesAccepted = { ROLE_COUNT, ROLE_COUNT, ROLE_COUNT, ROLE_COUNT,
                                                        ROLE_COUNT, ROLE_COUNT, ROLE_COUNT, ROLE_COUNT };
    Array<RoleEnum, MAX_ROLE_FILTER> rolesRejected = { ROLE_COUNT, ROLE_COUNT, ROLE_COUNT, ROLE_COUNT,
                                                        ROLE_COUNT, ROLE_COUNT, ROLE_COUNT, ROLE_COUNT };

    // ── item filter ────────────────────────────────────────────────────────────
    // Actor/Target: character must carry at least one item satisfying all active item conditions.
    // Tool: the tool item itself must satisfy these conditions.
    Array<ItemEnum, MAX_ITEM_FILTER> itemAccepted = { ITEM_UNALLOCATED, ITEM_UNALLOCATED, ITEM_UNALLOCATED, ITEM_UNALLOCATED,
                                                       ITEM_UNALLOCATED, ITEM_UNALLOCATED, ITEM_UNALLOCATED, ITEM_UNALLOCATED };
    Array<ItemEnum, MAX_ITEM_FILTER> itemRejected = { ITEM_UNALLOCATED, ITEM_UNALLOCATED, ITEM_UNALLOCATED, ITEM_UNALLOCATED,
                                                       ITEM_UNALLOCATED, ITEM_UNALLOCATED, ITEM_UNALLOCATED, ITEM_UNALLOCATED };
    TraitBits itemTraitsRequired   = {};
    TraitBits itemTraitsRestricted = {};

    // ── lock filter (Actor/Target only) ────────────────────────────────────────
    // Character must have an associated Chest. A character without a Chest fails when any lock filter is active.
    TraitBits lockTraitsRequired   = {};
    TraitBits lockTraitsRestricted = {};
    Array<LockEnum, MAX_LOCK_ACCEPTED> locksAccepted = { LOCK_COUNT, LOCK_COUNT, LOCK_COUNT, LOCK_COUNT,
                                                          LOCK_COUNT, LOCK_COUNT, LOCK_COUNT, LOCK_COUNT };
    Array<LockEnum, MAX_LOCK_REJECTED> locksRejected = { LOCK_COUNT, LOCK_COUNT, LOCK_COUNT,
                                                          LOCK_COUNT, LOCK_COUNT, LOCK_COUNT };
};

// Checks that memory[var] character is currently in the actor's room.
struct TriggerMatchCharacterVisible {
    ConductMemoryVariableEnum var = CONDUCT_MEMORY_CHARACTER_ID;
};

// Checks that memory[var] character is NOT in the actor's room.
struct TriggerMatchCharacterLost {
    ConductMemoryVariableEnum var = CONDUCT_MEMORY_CHARACTER_ID;
};

// Scans dungeon chests for one in actor's room satisfying match and condition.
// Use match.locks to filter by lock attributes (e.g. makeTraitBits({TRAIT_LOCK_OPEN})).
// Populates scannedObjectId (containerCharacterId) on success.
struct TriggerMatchScanObject {
    WrapperConfig::Match     match     = {};
    WrapperConfig::Condition condition = {};
};

// Checks that memory[var] object is in the actor's room.
struct TriggerMatchObjectVisible {
    ConductMemoryVariableEnum var = CONDUCT_MEMORY_ITEM_ID;
};

enum class TriggerMatchCharacterSource : int { Actor = 0, Target = 1 };

// Passes when the activation actor or target characterId equals memory[var].
// Returns false if memory[var] is -1 (unset) or (for Target) if no target character is present.
struct TriggerMatchCharacterId {
    ConductMemoryVariableEnum   var    = CONDUCT_MEMORY_CHARACTER_ID;
    TriggerMatchCharacterSource source = TriggerMatchCharacterSource::Actor;
};

// Checks the observing NPC (activation.observerCharacterId) against match and condition.
// Only meaningful in asObserver slots; falls back to activation.character when observerCharacterId == -1.
struct TriggerMatchObserver {
    WrapperConfig::Match     match     = {};
    WrapperConfig::Condition condition = {};
};

using TriggerMatch = std::variant<
    std::monostate,                      // empty slot — terminates match array scan
    TriggerMatchActor,
    TriggerMatchTool,
    TriggerMatchTarget,
    TriggerMatchReachedTargetRoom,
    TriggerMatchCharacter,
    TriggerMatchCharacterVisible,
    TriggerMatchCharacterLost,
    TriggerMatchScanObject,
    TriggerMatchObjectVisible,
    TriggerMatchCharacterId,
    TriggerMatchObserver
>;
