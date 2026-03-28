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
    auto& character = activation.character;
    auto& codeset = activation.codeset;
    auto& controller = activation.controller;
    auto& inventory = activation.player.inventory;

    // Check trait preconditions
    const auto computed = controller.getTraitsComputed(character.characterId).final;
    if (codeset.addFailure((_config.required & computed) != _config.required, CODE_WRAPPER_MISSING_REQUIRED_TRAITS)) return false;
    if (codeset.addFailure((_config.restricted & computed).isAny(), CODE_WRAPPER_HAS_RESTRICTED_TRAITS)) return false;

    // Dry-run item costs to ensure they can all be paid
    for (int i = 0; i < WrapperConfig::MAX_COSTS; i++) {
        if (_config.itemCost[i] == ITEM_NIL) break;
        if (codeset.addFailure(!controller.takeInventoryItem(inventory, _config.itemCost[i], true), CODE_WRAPPER_INSUFFICIENT_ITEM_COST)) return false;
    }

    // Commit item costs
    for (int i = 0; i < WrapperConfig::MAX_COSTS; i++) {
        if (_config.itemCost[i] == ITEM_NIL) break;
        if (codeset.addFailure(!controller.takeInventoryItem(inventory, _config.itemCost[i], activation.time, activation.room.roomId, activation.isSkippingAnimations), CODE_WRAPPER_FAILED_TO_TAKE_ITEM)) return false;
    }

    // Pay action costs
    for (int i = 0; i < _config.actionCost; i++) {
        if (codeset.addFailure(!controller.takeCharacterAction(character), CODE_WRAPPER_INSUFFICIENT_ACTIONS)) return false;
    }

    // Pay move costs
    for (int i = 0; i < _config.moveCost; i++) {
        if (codeset.addFailure(!controller.takeCharacterMove(character), CODE_WRAPPER_INSUFFICIENT_MOVES)) return false;
    }

    // Run effects in sequence; stop and return false on first failure
    for (int i = 0; i < WrapperConfig::MAX_EFFECTS; i++) {
        if (std::holds_alternative<NoEffect>(_config.effects[i])) break;
        bool result = std::visit([&](auto& effect) {
            return effect.activate(activation);
        }, _config.effects[i]);
        if (codeset.addFailure(!result, CODE_WRAPPER_EFFECT_FAILED)) return false;
    }
    return true;
}
