#include "ActivatorWrapper.hpp"
#include <variant>
#include "Character.hpp"
#include "Codeset.hpp"
#include "ItemEnum.hpp"
#include "MatchController.hpp"
#include "Player.hpp"
#include "Room.hpp"
#include "TraitModifier.hpp"

void ActivatorWrapper::init(const WrapperConfig& config) {
    _config = config;
}

bool ActivatorWrapper::activate(Activation& activation) const {
    bool result = false;
    activation.request.access([&](RequestContext& req) {
        auto& character = activation.character;
        auto& codeset = req.codeset;
        auto& controller = req.controller;
        auto& inventory = req.player.inventory;

        const auto computed = controller.getTraitsComputed(character.characterId).final;
        if (codeset.addFailure((_config.required & computed) != _config.required, CODE_WRAPPER_MISSING_REQUIRED_TRAITS)) return;
        if (codeset.addFailure((_config.restricted & computed).isAny(), CODE_WRAPPER_HAS_RESTRICTED_TRAITS)) return;

        for (int i = 0; i < WrapperConfig::MAX_COSTS; i++) {
            if (_config.itemCost[i] == ITEM_NIL) break;
            if (codeset.addFailure(!controller.takeInventoryItem(inventory, _config.itemCost[i], true), CODE_WRAPPER_INSUFFICIENT_ITEM_COST)) return;
        }

        for (int i = 0; i < WrapperConfig::MAX_COSTS; i++) {
            if (_config.itemCost[i] == ITEM_NIL) break;
            if (codeset.addFailure(!controller.takeInventoryItem(inventory, _config.itemCost[i], req.time, req.room.roomId, req.isSkippingAnimations), CODE_WRAPPER_FAILED_TO_TAKE_ITEM)) return;
        }

        for (int i = 0; i < _config.actionCost; i++) {
            if (codeset.addFailure(!controller.takeCharacterAction(character), CODE_WRAPPER_INSUFFICIENT_ACTIONS)) return;
        }

        for (int i = 0; i < _config.moveCost; i++) {
            if (codeset.addFailure(!controller.takeCharacterMove(character), CODE_WRAPPER_INSUFFICIENT_MOVES)) return;
        }

        for (int i = 0; i < WrapperConfig::MAX_EFFECTS; i++) {
            if (std::holds_alternative<NoEffect>(_config.effects[i])) break;
            bool effectResult = std::visit([&](auto& effect) {
                return effect.activate(activation);
            }, _config.effects[i]);
            if (codeset.addFailure(!effectResult, CODE_WRAPPER_EFFECT_FAILED)) return;
        }
        result = true;
    });
    return result;
}
