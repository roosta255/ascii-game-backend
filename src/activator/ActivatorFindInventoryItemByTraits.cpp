#include "ActivatorFindInventoryItemByTraits.hpp"
#include "Character.hpp"
#include "ItemEnum.hpp"
#include "MatchController.hpp"

bool ActivatorFindInventoryItemByTraits::activate(ActivationContext& activation) const {
    bool result = false;
    activation.request.access([&](RequestContext& req) {
        auto search = [&](Character& character) {
            auto inventory = character.getInventory(req.match.dungeon);
            if (!inventory.isValid()) return;

            ItemEnum found = ITEM_NIL;
            inventory.accessItemByTraits(required, restricted, [&](const Item& item) { found = item.type; });
            if (found == ITEM_NIL) return;

            activation.resolvedItem = found;
            result = true;
        };

        if (side == CharacterAnchor::Actor) {
            search(activation.character);
        } else {
            activation.targetCharacter().access([&](Character& target) { search(target); });
        }
    });
    return result;
}
