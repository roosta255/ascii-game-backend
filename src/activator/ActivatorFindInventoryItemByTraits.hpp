#pragma once

#include "CharacterRelation.hpp"
#include "TraitBits.hpp"
#include "iActivator.hpp"

// Decider: searches `side`'s inventory for an item whose flyweight traits satisfy
// required/restricted (see Inventory::accessItemByTraits) and, if found, stashes it
// on ActivationContext::resolvedItem for later effects (e.g. ActivatorTransferItem)
// to consume. Returns false — silently, no codeset failure — when the anchor doesn't
// resolve to a character, its inventory is unusable, or nothing matches.
class ActivatorFindInventoryItemByTraits : public iActivator {
public:
    ActivatorFindInventoryItemByTraits() = default;
    ActivatorFindInventoryItemByTraits(CharacterAnchor side, TraitBits required, TraitBits restricted = {})
        : side(side), required(required), restricted(restricted) {}

    CharacterAnchor side = CharacterAnchor::Target;
    TraitBits required = {};
    TraitBits restricted = {};

    bool activate(ActivationContext& activation) const override;
};
