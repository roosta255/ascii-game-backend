#include "ActivatorDeposit.hpp"
#include "ActivationContext.hpp"
#include "BehaviorEventEnum.hpp"
#include "Codeset.hpp"
#include "EventFlyweight.hpp"
#include "ItemEnum.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "TraitBits.hpp"
#include "TraitEnum.hpp"

bool ActivatorDeposit::activate(ActivationContext& activation) const {
    bool result = false;
    activation.request.access([&](RequestContext& req) {
        auto& controller  = req.controller;
        auto& depositor   = activation.character;
        const int roomId  = activation.room.roomId;

        const auto traits = controller.getTraitsComputed(depositor.characterId).final;
        if (!traits[TRAIT_HANDS].orElse(false)) return;

        activation.targetCharacter().access([&](Character& chest) {
            auto actorInventory = depositor.getInventory(req.match.dungeon);
            if (!actorInventory.isValid()) return;

            ItemEnum itemToDeposit = ITEM_NIL;
            actorInventory.accessItemByTraits(
                makeTraitBits({ TRAIT_ITEM_TRANSFERABLE, TRAIT_PICKPOCKETABLE }),
                makeTraitBits({}),
                [&](const Item& item) { itemToDeposit = item.type; }
            );
            if (itemToDeposit == ITEM_NIL) return;

            auto chestInventory = chest.getInventory(req.match.dungeon);
            if (!chestInventory.isValid()) return;

            if (!controller.takeCharacterAction(depositor)) return;
            if (!controller.takeInventoryItem(actorInventory, itemToDeposit)) return;
            if (!controller.giveInventoryItem(chestInventory, itemToDeposit)) return;

            controller.updateTraits(depositor);
            result = true;

            controller.addLoggedEvent(activation, roomId, LoggedEvent{
                EVENT_DEPOSIT_LOOT,
                { EventComponentKind::ROLE, (int)depositor.role },
                {},
                { EventComponentKind::ITEM, (int)itemToDeposit },
                -1
            });

            controller.pushTrigger(nullptr, depositor.characterId, chest.characterId, BEHAVIOR_EVENT_DEPOSIT);
        });
    });
    return result;
}
