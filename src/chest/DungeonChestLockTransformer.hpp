#pragma once

#include "LockEnum.hpp"
#include "TraitBits.hpp"
#include "iActivator.hpp"

// Determines which chests the transformer visits.
enum class ChestScope {
    Subject,  // chest whose containerCharacterId matches the actor character
    Target,   // chest whose containerCharacterId matches the target character
    Room,     // all chests whose container character is in the actor's current room
    Dungeon   // all chests in the dungeon
};

// Determines the new LockEnum to assign to each visited chest.
//
// Direct mode: directLock != LOCK_NIL — assigns that specific lock unconditionally.
// Trait mode:  directLock == LOCK_NIL — applies a TraitBits delta to the current
//              lock's lockAttributes and resolves the result via
//              LockFlyweight::findByTraits. Chests whose current lock lacks any bit
//              in `match` are skipped; chests where no LockEnum matches the computed
//              traits are left unchanged.
struct LockAssignment {
    LockEnum  directLock = LOCK_NIL;  // non-NIL → direct mode
    TraitBits match      = {};        // trait mode: current lock must have all these bits
    TraitBits set        = {};        // trait mode: bits to add to current lockAttributes
    TraitBits clear      = {};        // trait mode: bits to remove from current lockAttributes
};

// Visits chests within the given scope and transitions each chest's lock according
// to the LockAssignment. Analogous to ActivatorSetAllDoorsByTrait but for chests.
class DungeonChestLockTransformer : public iActivator {
public:
    DungeonChestLockTransformer() = default;
    DungeonChestLockTransformer(ChestScope scope, LockAssignment assignment)
        : scope(scope), assignment(assignment) {}

    ChestScope     scope      = ChestScope::Dungeon;
    LockAssignment assignment = {};

    bool activate(ActivationContext& activation) const override;
};
