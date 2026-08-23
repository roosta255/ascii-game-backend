#pragma once

#include <variant>
#include "ActionEnum.hpp"
#include "Array.hpp"
#include "ConductMemoryVariableEnum.hpp"
#include "LockEnum.hpp"
#include "WrapperConfig.hpp"

// BehaviorEnum is forward-declared to avoid a circular include.
// By the time Behavior.enum is expanded in BehaviorFlyweight.cpp, BehaviorEnum
// is already fully defined via BehaviorFlyweight.hpp → BehaviorEnum.hpp.
enum BehaviorEnum : int;

// Effects applied to ConductMemory (and optionally character state) when a
// TriggerWrapper fires.
//
// Variable-write effects read a "scanned" value populated by a preceding match:
//   SetVar           — scannedCharacterId → memory[var]   (from ScanCharacter / ScanRoomForCharacter)
//   SetVarFromObj    — scannedObjectId    → memory[var]   (from ScanObject)
//   SetVarFromRoom   — scannedRoomId      → memory[var]   (from room scan, currently unused)
//   SetVarFromContext — activation.targetCharacter() id → memory[var]
//   ClearVar         — memory[var] = -1

struct TriggerEffectSetBehavior       { BehaviorEnum behavior; };
struct TriggerEffectSetVar            { ConductMemoryVariableEnum var; };
struct TriggerEffectSetVarFromObj     { ConductMemoryVariableEnum var; };
struct TriggerEffectSetVarFromRoom    { ConductMemoryVariableEnum var; };
struct TriggerEffectSetVarFromContext { ConductMemoryVariableEnum var; };
struct TriggerEffectClearVar          { ConductMemoryVariableEnum var; };

// TriggerInnerEffect — all effect types that may appear nested inside
// TriggerEffectScanRoomForCharacter::onFound.
using TriggerInnerEffect = std::variant<
    std::monostate,
    TriggerEffectSetBehavior,
    TriggerEffectSetVar,
    TriggerEffectSetVarFromObj,
    TriggerEffectSetVarFromRoom,
    TriggerEffectSetVarFromContext,
    TriggerEffectClearVar
>;

// Scans all room characters (builders + dungeon characters) for the first one
// satisfying match and condition. On success, applies onFound effects with
// scannedCharacterId set to the found character's id. Does nothing on no match.
struct TriggerEffectScanRoomForCharacter {
    static constexpr int MAX_ON_FOUND = 4;
    WrapperConfig::Match     match     = {};
    WrapperConfig::Condition condition = {};
    Array<TriggerInnerEffect, MAX_ON_FOUND> onFound = {};
};

// Full character scan across builders and/or dungeon character sets.
// Scope controls narrow which characters are visited; the trait/role filter
// splits visitors into "matched" vs "rejected". All output slots are
// independent: use CONDUCT_MEMORY_VARIABLE_COUNT to skip writing any slot.
//
// Item filter: if any item field is non-empty/non-zero, a character must carry
// at least one item satisfying ALL active item conditions to pass the filter.
//
// Lock filter: if any lock field is non-empty/non-zero, a character must have
// an associated Chest (via chestContainerMap). A character without a Chest
// automatically fails when any lock filter is active.
// lockTraitsRequired/lockTraitsRestricted check the Chest's lock's lockAttributes.
// locksAccepted/locksRejected check the Chest's lock enum directly.
//
// Room-bit outputs accumulate bits for rooms 0–31 only (int width).
// unmatchedRoomBitsVar = scanned rooms with zero matches.
// unrejectedRoomBitsVar = scanned rooms with zero rejections.
//
// Role, item, and lock arrays are sentinel-terminated (ROLE_COUNT / ITEM_UNALLOCATED / LOCK_COUNT);
// an empty array means no constraint.
struct TriggerEffectScanCharacters {
    static constexpr int MAX_ROLE_FILTER    = 8;
    static constexpr int MAX_ITEM_FILTER    = 8;
    static constexpr int MAX_LOCK_ACCEPTED  = 8;
    static constexpr int MAX_LOCK_REJECTED  = 6;

    // ── scope ──────────────────────────────────────────────────────────────────
    bool scanBuilders  = true;
    bool scanDungeon   = true;

    bool onlyObserver          = false;  // restrict scan to the conducting character only
    bool ignoreSelf        = true;   // skip the conducting character
    bool ignoreInRoom      = false;  // skip characters in actor's room
    bool ignoreOutsideRoom = false;  // skip characters outside actor's room

    // Read a ROOM_BITS memory variable to further restrict scope.
    // CONDUCT_MEMORY_VARIABLE_COUNT = not configured.
    ConductMemoryVariableEnum roomsIgnoredVar = CONDUCT_MEMORY_VARIABLE_COUNT; // skip chars whose room bit is set
    ConductMemoryVariableEnum roomsScannedVar = CONDUCT_MEMORY_VARIABLE_COUNT; // only visit chars whose room bit is set

    // ── character filter ───────────────────────────────────────────────────────
    TraitBits traitsRequired   = {};
    TraitBits traitsRestricted = {};
    Array<RoleEnum, MAX_ROLE_FILTER> rolesAccepted = { ROLE_COUNT, ROLE_COUNT, ROLE_COUNT, ROLE_COUNT,
                                                        ROLE_COUNT, ROLE_COUNT, ROLE_COUNT, ROLE_COUNT };
    Array<RoleEnum, MAX_ROLE_FILTER> rolesRejected = { ROLE_COUNT, ROLE_COUNT, ROLE_COUNT, ROLE_COUNT,
                                                        ROLE_COUNT, ROLE_COUNT, ROLE_COUNT, ROLE_COUNT };

    // ── item filter (optional — any active field requires a matching carried item) ──
    // ITEM_UNALLOCATED-terminated; empty list = no item-type constraint.
    Array<ItemEnum, MAX_ITEM_FILTER> itemAccepted = { ITEM_UNALLOCATED, ITEM_UNALLOCATED, ITEM_UNALLOCATED, ITEM_UNALLOCATED,
                                                       ITEM_UNALLOCATED, ITEM_UNALLOCATED, ITEM_UNALLOCATED, ITEM_UNALLOCATED };
    Array<ItemEnum, MAX_ITEM_FILTER> itemRejected = { ITEM_UNALLOCATED, ITEM_UNALLOCATED, ITEM_UNALLOCATED, ITEM_UNALLOCATED,
                                                       ITEM_UNALLOCATED, ITEM_UNALLOCATED, ITEM_UNALLOCATED, ITEM_UNALLOCATED };
    TraitBits itemTraitsRequired   = {};
    TraitBits itemTraitsRestricted = {};

    // ── lock filter (optional — any active field requires a corresponding Chest) ──
    // lockTraitsRequired/lockTraitsRestricted operate on the Chest lock's lockAttributes.
    // LOCK_COUNT-terminated; empty list = no lock-enum constraint.
    TraitBits lockTraitsRequired   = {};
    TraitBits lockTraitsRestricted = {};
    Array<LockEnum, MAX_LOCK_ACCEPTED> locksAccepted = { LOCK_COUNT, LOCK_COUNT, LOCK_COUNT, LOCK_COUNT,
                                                          LOCK_COUNT, LOCK_COUNT, LOCK_COUNT, LOCK_COUNT };
    Array<LockEnum, MAX_LOCK_REJECTED> locksRejected = { LOCK_COUNT, LOCK_COUNT, LOCK_COUNT,
                                                          LOCK_COUNT, LOCK_COUNT, LOCK_COUNT };

    // ── outputs ────────────────────────────────────────────────────────────────
    // CONDUCT_MEMORY_VARIABLE_COUNT = don't write this slot.
    // Role outputs store the raw RoleEnum cast to int.
    ConductMemoryVariableEnum firstMatchedCharId    = CONDUCT_MEMORY_VARIABLE_COUNT;
    ConductMemoryVariableEnum firstMatchedRoomId    = CONDUCT_MEMORY_VARIABLE_COUNT;
    ConductMemoryVariableEnum firstMatchedRole      = CONDUCT_MEMORY_VARIABLE_COUNT;
    ConductMemoryVariableEnum firstRejectedCharId   = CONDUCT_MEMORY_VARIABLE_COUNT;
    ConductMemoryVariableEnum firstRejectedRoomId   = CONDUCT_MEMORY_VARIABLE_COUNT;
    ConductMemoryVariableEnum firstRejectedRole     = CONDUCT_MEMORY_VARIABLE_COUNT;
    ConductMemoryVariableEnum matchedRoomBitsVar    = CONDUCT_MEMORY_VARIABLE_COUNT;
    ConductMemoryVariableEnum unmatchedRoomBitsVar  = CONDUCT_MEMORY_VARIABLE_COUNT;
    ConductMemoryVariableEnum rejectedRoomBitsVar   = CONDUCT_MEMORY_VARIABLE_COUNT;
    ConductMemoryVariableEnum unrejectedRoomBitsVar = CONDUCT_MEMORY_VARIABLE_COUNT;
};

// Runs A* from the conducting character (or a character from memory) to a
// target room or set of rooms, writing path data into configurable memory slots.
//
// Goal: set targetRoomVar (ROOM_ID) OR targetBitsVar (ROOM_BITS); the search
// stops when the subject reaches that room or any room whose bit is set.
//
// seenRoomBitsVar:      rooms evaluated by the heuristic (the explored frontier).
// traversedRoomBitsVar: rooms from which an action is taken in the found path.
// doorNorth/East/South/WestVar: per-room direction bits for TriggerEffectTraverseDoor.
//   Bit N set in doorNorthVar means "move north when in room N."
//
// loops = 0 uses DEFAULT_LOOPS. CONDUCT_MEMORY_VARIABLE_COUNT = don't write/read.
struct TriggerEffectComputePath {
    static constexpr int DEFAULT_LOOPS = 256;

    ConductMemoryVariableEnum characterVar     = CONDUCT_MEMORY_VARIABLE_COUNT; // CHARACTER_ID; actor if unset

    ConductMemoryVariableEnum targetRoomVar    = CONDUCT_MEMORY_VARIABLE_COUNT; // ROOM_ID goal
    ConductMemoryVariableEnum targetBitsVar    = CONDUCT_MEMORY_VARIABLE_COUNT; // ROOM_BITS goal

    int loops = 0;

    ConductMemoryVariableEnum seenRoomBitsVar      = CONDUCT_MEMORY_VARIABLE_COUNT;
    ConductMemoryVariableEnum traversedRoomBitsVar = CONDUCT_MEMORY_VARIABLE_COUNT;
    ConductMemoryVariableEnum doorNorthVar         = CONDUCT_MEMORY_VARIABLE_COUNT;
    ConductMemoryVariableEnum doorEastVar          = CONDUCT_MEMORY_VARIABLE_COUNT;
    ConductMemoryVariableEnum doorSouthVar         = CONDUCT_MEMORY_VARIABLE_COUNT;
    ConductMemoryVariableEnum doorWestVar          = CONDUCT_MEMORY_VARIABLE_COUNT;
};

// Runs a BFS flood fill from a target room or set of rooms, writing two groups
// of per-room door-direction bits into configurable memory slots.
//
// Start: set startRoomVar (ROOM_ID) OR startBitsVar (ROOM_BITS); the fill
// expands from that room or every room whose bit is set.
// CONDUCT_MEMORY_VARIABLE_COUNT = don't read/write a slot.
//
// Towards outputs — for each discovered transition (sourceRoom → dir → destRoom),
// bit sourceRoom is set in the towards-<dir> variable.
// Example: "go North from room 5 to reach origin" → bit 5 set in towardsNorthVar.
//
// Against outputs — bit destRoom is set in the against-<flippedDir> variable.
// Example: "North from room 5 leads into room 8" → bit 8 set in againstSouthVar,
// marking room 8 as a room where going South leads away from the origin.
struct TriggerEffectComputeFloodPath {
    ConductMemoryVariableEnum startRoomVar    = CONDUCT_MEMORY_VARIABLE_COUNT; // ROOM_ID origin
    ConductMemoryVariableEnum startBitsVar    = CONDUCT_MEMORY_VARIABLE_COUNT; // ROOM_BITS origin

    ConductMemoryVariableEnum towardsNorthVar = CONDUCT_MEMORY_VARIABLE_COUNT;
    ConductMemoryVariableEnum towardsEastVar  = CONDUCT_MEMORY_VARIABLE_COUNT;
    ConductMemoryVariableEnum towardsSouthVar = CONDUCT_MEMORY_VARIABLE_COUNT;
    ConductMemoryVariableEnum towardsWestVar  = CONDUCT_MEMORY_VARIABLE_COUNT;

    ConductMemoryVariableEnum againstNorthVar = CONDUCT_MEMORY_VARIABLE_COUNT;
    ConductMemoryVariableEnum againstEastVar  = CONDUCT_MEMORY_VARIABLE_COUNT;
    ConductMemoryVariableEnum againstSouthVar = CONDUCT_MEMORY_VARIABLE_COUNT;
    ConductMemoryVariableEnum againstWestVar  = CONDUCT_MEMORY_VARIABLE_COUNT;
};

// Reads activation.direction as a Cardinal index (0=N 1=E 2=S 3=W) into memory[var].
// Writes -1 if direction is absent on the activation.
struct TriggerEffectSetVarFromDirection { ConductMemoryVariableEnum var; };

// Reads the conducting character's current room id → memory[var].
struct TriggerEffectSetVarFromSelfRoom  { ConductMemoryVariableEnum var; };

// Reads memory[var] as a Cardinal index and assigns it to activation.direction.
// No-op if the stored value is outside [0, 3].
struct TriggerEffectSetDirectionFromVar { ConductMemoryVariableEnum var; };

// Reads doorTraversal bits from memory, resolves the direction for the actor's
// current room, and attempts ACTION_MOVE_TO_DOOR followed by ACTION_ACTIVATE_DOOR
// if movement is rejected (for ladders, poles, etc.).
//
// Cardinal order matches the memory slots: north=0, east=1, south=2, west=3.
// CONDUCT_MEMORY_VARIABLE_COUNT = that direction is not configured.
struct TriggerEffectTraverseDoor {
    ConductMemoryVariableEnum doorNorthVar = CONDUCT_MEMORY_VARIABLE_COUNT;
    ConductMemoryVariableEnum doorEastVar  = CONDUCT_MEMORY_VARIABLE_COUNT;
    ConductMemoryVariableEnum doorSouthVar = CONDUCT_MEMORY_VARIABLE_COUNT;
    ConductMemoryVariableEnum doorWestVar  = CONDUCT_MEMORY_VARIABLE_COUNT;
};

// Execute an action from the conducting character. Works in both trigger and
// proposal contexts. targetVar reads a CHARACTER_ID memory slot (CONDUCT_MEMORY_VARIABLE_COUNT = none).
// toolVar reads an ITEM_ID memory slot (CONDUCT_MEMORY_VARIABLE_COUNT = none).
struct TriggerEffectExecuteAction {
    ActionEnum                action    = ACTION_NIL;
    ConductMemoryVariableEnum targetVar = CONDUCT_MEMORY_VARIABLE_COUNT;
    ConductMemoryVariableEnum toolVar   = CONDUCT_MEMORY_VARIABLE_COUNT;
};

using TriggerEffect = std::variant<
    std::monostate,
    TriggerEffectSetBehavior,
    TriggerEffectSetVar,
    TriggerEffectSetVarFromObj,
    TriggerEffectSetVarFromRoom,
    TriggerEffectSetVarFromContext,
    TriggerEffectClearVar,
    TriggerEffectScanRoomForCharacter,
    TriggerEffectScanCharacters,
    TriggerEffectComputePath,
    TriggerEffectComputeFloodPath,
    TriggerEffectTraverseDoor,
    TriggerEffectSetVarFromDirection,
    TriggerEffectSetVarFromSelfRoom,
    TriggerEffectSetDirectionFromVar,
    TriggerEffectExecuteAction
>;
