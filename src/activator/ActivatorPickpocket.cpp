#include "ActivatorPickpocket.hpp"
#include "ActivationContext.hpp"
#include "BehaviorEventEnum.hpp"
#include "Codeset.hpp"
#include "EventFlyweight.hpp"
#include "ItemEnum.hpp"
#include "MatchController.hpp"
#include "TraitBits.hpp"
#include "TraitEnum.hpp"

bool ActivatorPickpocket::activate(ActivationContext& activation) const {
    bool result = false;
    activation.request.access([&](RequestContext& req) {
        auto& controller   = req.controller;
        auto& pickpocketer = activation.character;
        const int roomId   = activation.room.roomId;

        const auto pickerTraits = controller.getTraitsComputed(pickpocketer.characterId).final;
        if (!pickerTraits[TRAIT_PICKPOCKETER].orElse(false)) return;
        if (!pickerTraits[TRAIT_HANDS].orElse(false)) return;

        activation.targetCharacter().access([&](Character& victim) {
            const auto victimTraits = controller.getTraitsComputed(victim.characterId).final;
            if (!victimTraits[TRAIT_PICKPOCKETABLE].orElse(false)) return;
            // if (victimTraits[TRAIT_ACTION_READY].orElse(false)) return;

            auto victimInventory = victim.getInventory(req.match.dungeon);
            if (!victimInventory.isValid()) return;

            ItemEnum stolenItem = ITEM_NIL;
            victimInventory.accessItemByTraits(
                makeTraitBits({ TRAIT_ITEM_TRANSFERABLE, TRAIT_PICKPOCKETABLE }),
                makeTraitBits({}),
                [&](const Item& item) { stolenItem = item.type; }
            );
            if (stolenItem == ITEM_NIL) return;

            if (!controller.takeCharacterAction(pickpocketer)) return;
            if (!controller.takeInventoryItem(victimInventory, stolenItem)) return;

            // Place the stolen item in the pickpocketer's own inventory.
            auto actorInventory = pickpocketer.getInventory(req.match.dungeon);
            if (actorInventory.isValid())
                controller.giveInventoryItem(actorInventory, stolenItem);

            controller.updateTraits(pickpocketer);
            result = true;

            controller.addLoggedEvent(activation, roomId, LoggedEvent{
                EVENT_PICKPOCKET,
                { EventComponentKind::ROLE, (int)pickpocketer.role },
                {},
                { EventComponentKind::ITEM, (int)stolenItem },
                -1
            });

            controller.pushTrigger(nullptr, pickpocketer.characterId, victim.characterId, BEHAVIOR_EVENT_PICKPOCKET);
        });
    });
    return result;
}
