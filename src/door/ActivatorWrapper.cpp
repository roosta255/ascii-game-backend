#include "ActivatorWrapper.hpp"
#include <variant>
#include "Character.hpp"
#include "Codeset.hpp"
#include "ItemEnum.hpp"
#include "LoggedEvent.hpp"
#include "MatchController.hpp"
#include "Player.hpp"
#include "Room.hpp"
#include "TraitEnum.hpp"
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

        const TraitBits missingTraits = _config.required - computed;
        if (codeset.addFailure(missingTraits.isAny(), CODE_WRAPPER_MISSING_REQUIRED_TRAITS)) {
            for (int i = 0; i < TRAIT_COUNT; i++) {
                if (missingTraits[i].orElse(false)) {
                    controller.addRequestLoggedEvent(activation, LoggedEvent{
                        EVENT_MISSING_TRAIT,
                        { EventComponentKind::ROLE, (int)character.role },
                        {},
                        { EventComponentKind::TRAIT, i },
                        -1
                    });
                }
            }
            return;
        }

        const TraitBits restrictedTraits = _config.restricted & computed;
        if (codeset.addFailure(restrictedTraits.isAny(), CODE_WRAPPER_HAS_RESTRICTED_TRAITS)) {
            for (int i = 0; i < TRAIT_COUNT; i++) {
                if (restrictedTraits[i].orElse(false)) {
                    controller.addRequestLoggedEvent(activation, LoggedEvent{
                        EVENT_RESTRICTED_TRAIT,
                        { EventComponentKind::ROLE, (int)character.role },
                        {},
                        { EventComponentKind::TRAIT, i },
                        -1
                    });
                }
            }
            return;
        }

        for (int i = 0; i < WrapperConfig::MAX_COSTS; i++) {
            if (_config.itemCost[i] == ITEM_NIL) break;
            if (codeset.addFailure(!controller.takeInventoryItem(inventory, _config.itemCost[i], true), CODE_WRAPPER_INSUFFICIENT_ITEM_COST)) {
                controller.addRequestLoggedEvent(activation, LoggedEvent{
                    EVENT_MISSING_ITEM,
                    { EventComponentKind::ROLE, (int)character.role },
                    {},
                    { EventComponentKind::ITEM, (int)_config.itemCost[i] },
                    -1
                });
                return;
            }
        }

        for (int i = 0; i < WrapperConfig::MAX_COSTS; i++) {
            if (_config.itemCost[i] == ITEM_NIL) break;
            if (codeset.addFailure(!controller.takeInventoryItem(inventory, _config.itemCost[i], req.time, req.room.roomId, req.isSkippingAnimations), CODE_WRAPPER_FAILED_TO_TAKE_ITEM)) return;
        }

        for (int i = 0; i < _config.actionCost; i++) {
            if (codeset.addFailure(!controller.takeCharacterAction(character), CODE_WRAPPER_INSUFFICIENT_ACTIONS)) {
                controller.addRequestLoggedEvent(activation, LoggedEvent{
                    EVENT_NO_ACTIONS,
                    { EventComponentKind::ROLE, (int)character.role },
                    {},
                    {},
                    -1
                });
                return;
            }
        }

        for (int i = 0; i < _config.moveCost; i++) {
            if (codeset.addFailure(!controller.takeCharacterMove(character), CODE_WRAPPER_INSUFFICIENT_MOVES)) {
                controller.addRequestLoggedEvent(activation, LoggedEvent{
                    EVENT_NO_MOVES,
                    { EventComponentKind::ROLE, (int)character.role },
                    {},
                    {},
                    -1
                });
                return;
            }
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
