#pragma once

#include "CharacterRelation.hpp"
#include "ItemEnum.hpp"
#include "iActivator.hpp"

// Moves one item from `from`'s inventory to `to`'s inventory and refreshes computed
// traits on both ends. The item is either the static `item` field, or — when
// useResolvedItem is set — whatever ActivationContext::resolvedItem holds (as left
// there by a prior decider such as ActivatorFindInventoryItemByTraits). Fails (no
// partial state left behind — the take is rolled back) if either side doesn't
// resolve to a character with a usable inventory, or the destination can't accept
// the item.
class ActivatorTransferItem : public iActivator {
public:
    ActivatorTransferItem() = default;
    ActivatorTransferItem(CharacterAnchor from, CharacterAnchor to, ItemEnum item)
        : from(from), to(to), item(item) {}
    static ActivatorTransferItem resolved(CharacterAnchor from, CharacterAnchor to) {
        ActivatorTransferItem effect(from, to, ITEM_NIL);
        effect.useResolvedItem = true;
        return effect;
    }

    CharacterAnchor from = CharacterAnchor::Target;
    CharacterAnchor to = CharacterAnchor::Actor;
    ItemEnum item = ITEM_NIL;
    bool useResolvedItem = false;

    bool activate(ActivationContext& activation) const override;
};
