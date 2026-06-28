#pragma once

#include "RoleEnum.hpp"
#include "TraitBits.hpp"
#include "iActivator.hpp"

// Determines which characters the transformer visits.
enum class CharacterScope {
    Object,   // the activating character
    Target,   // the target character
    Tool,     // the character whose inventory contains sourceItem
    Room,     // all dungeon characters in the actor's current room
    Dungeon   // all dungeon characters
};

// Determines the new RoleEnum to assign to each visited character.
//
// Direct mode: directRole != ROLE_COUNT — assigns that specific role unconditionally.
// Trait mode:  directRole == ROLE_COUNT — applies a TraitBits delta to the current
//              role's traitsSourced and resolves the result via
//              RoleFlyweight::findByTraits. Characters whose current role lacks any
//              bit in `match` are skipped; characters where no RoleEnum matches the
//              computed traits are left unchanged.
struct RoleAssignment {
    RoleEnum  directRole = ROLE_COUNT;  // non-COUNT → direct mode
    TraitBits match      = {};          // trait mode: role's traitsSourced must have all these bits
    TraitBits set        = {};          // trait mode: bits to add to current traitsSourced
    TraitBits clear      = {};          // trait mode: bits to remove from current traitsSourced
};

// Visits characters within the given scope and transitions each character's role
// according to the RoleAssignment. Analogous to DungeonChestLockTransformer but
// for character roles.
class DungeonRoleTransformer : public iActivator {
public:
    DungeonRoleTransformer() = default;
    DungeonRoleTransformer(CharacterScope scope, RoleAssignment assignment)
        : scope(scope), assignment(assignment) {}

    CharacterScope scope      = CharacterScope::Dungeon;
    RoleAssignment assignment = {};

    bool activate(ActivationContext& activation) const override;
};
