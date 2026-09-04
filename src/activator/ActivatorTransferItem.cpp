#include "ActivatorTransferItem.hpp"
#include "Character.hpp"
#include "MatchController.hpp"
#include "Pointer.hpp"

bool ActivatorTransferItem::activate(ActivationContext& activation) const {
    bool result = false;
    activation.request.access([&](RequestContext& req) {
        auto& controller = req.controller;

        const ItemEnum itemToMove = useResolvedItem ? activation.resolvedItem.orElse(ITEM_NIL) : item;
        if (itemToMove == ITEM_NIL) return;

        auto resolve = [&](CharacterAnchor anchor) -> Pointer<Character> {
            return anchor == CharacterAnchor::Actor ? Pointer<Character>(activation.character) : activation.targetCharacter();
        };

        resolve(from).access([&](Character& fromCharacter) {
            resolve(to).access([&](Character& toCharacter) {
                auto fromInventory = fromCharacter.getInventory(req.match.dungeon);
                auto toInventory = toCharacter.getInventory(req.match.dungeon);
                if (!fromInventory.isValid() || !toInventory.isValid()) return;

                if (!controller.takeInventoryItem(fromInventory, itemToMove)) return;
                if (!controller.giveInventoryItem(toInventory, itemToMove)) {
                    // Destination couldn't accept it — put it back rather than losing it.
                    controller.giveInventoryItem(fromInventory, itemToMove);
                    return;
                }

                controller.updateTraits(fromCharacter);
                controller.updateTraits(toCharacter);
                result = true;
            });
        });
    });
    return result;
}
