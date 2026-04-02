#include "ActivatorWrapper.hpp"
#include <variant>
#include "Character.hpp"
#include "Codeset.hpp"
#include "DoorEnum.hpp"
#include "DoorFlyweight.hpp"
#include "ItemEnum.hpp"
#include "ItemFlyweight.hpp"
#include "LoggedEvent.hpp"
#include "MatchController.hpp"
#include "Player.hpp"
#include "RoleEnum.hpp"
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

        // === Match phase (rule selection — no failure logged on mismatch) ===

        // Actor is always a character — only traits and roles lists are checked.
        if (_config.matches.actor.isAny()) {
            const auto& m = _config.matches.actor;
            bool matched = true;
            if (m.traits.isAny())
                matched = !(m.traits - computed).isAny();
            if (matched && m.roles[0] != ROLE_COUNT) {
                bool found = false;
                for (int i = 0; i < WrapperConfig::MAX_MATCH_LIST && m.roles[i] != ROLE_COUNT; i++) {
                    if (character.role == m.roles[i]) { found = true; break; }
                }
                matched = found;
            }
            if (!matched) return;
        }

        // Tool is always an item — only traits and items lists are checked.
        if (_config.matches.tool.isAny()) {
            bool toolMatched = false;
            activation.sourceItem.access([&](Item& item) {
                item.accessFlyweight([&](const ItemFlyweight& fw) {
                    const auto& m = _config.matches.tool;
                    bool matched = true;
                    if (m.traits.isAny())
                        matched = !(m.traits - fw.itemAttributes).isAny();
                    if (matched && m.items[0] != ITEM_NIL) {
                        bool found = false;
                        for (int i = 0; i < WrapperConfig::MAX_MATCH_LIST && m.items[i] != ITEM_NIL; i++) {
                            if (item.type == m.items[i]) { found = true; break; }
                        }
                        matched = found;
                    }
                    toolMatched = matched;
                });
            });
            if (!toolMatched) return;
        }

        // Target can be a character, item, or wall — list type checked against actual target type.
        if (_config.matches.target.isAny()) {
            const auto& m = _config.matches.target;
            bool targetMatched = false;
            bool targetFound = false;
            activation.targetCharacter().access([&](Character& target) {
                targetFound = true;
                const auto targetComputed = controller.getTraitsComputed(target.characterId).final;
                bool matched = true;
                if (m.traits.isAny())
                    matched = !(m.traits - targetComputed).isAny();
                if (matched && m.roles[0] != ROLE_COUNT) {
                    bool found = false;
                    for (int i = 0; i < WrapperConfig::MAX_MATCH_LIST && m.roles[i] != ROLE_COUNT; i++) {
                        if (target.role == m.roles[i]) { found = true; break; }
                    }
                    matched = found;
                }
                targetMatched = matched;
            });
            if (!targetFound) {
                activation.targetItem().access([&](Item& item) {
                    targetFound = true;
                    item.accessFlyweight([&](const ItemFlyweight& fw) {
                        bool matched = true;
                        if (m.traits.isAny())
                            matched = !(m.traits - fw.itemAttributes).isAny();
                        if (matched && m.items[0] != ITEM_NIL) {
                            bool found = false;
                            for (int i = 0; i < WrapperConfig::MAX_MATCH_LIST && m.items[i] != ITEM_NIL; i++) {
                                if (item.type == m.items[i]) { found = true; break; }
                            }
                            matched = found;
                        }
                        targetMatched = matched;
                    });
                });
            }
            if (!targetFound) {
                activation.targetWall().access([&](Wall& w) {
                    DoorFlyweight::getFlyweights().accessConst((int)w.door, [&](const DoorFlyweight& fw) {
                        bool matched = true;
                        if (m.traits.isAny())
                            matched = !(m.traits - fw.doorAttributes).isAny();
                        if (matched && m.doors[0] != DOOR_COUNT) {
                            bool found = false;
                            for (int i = 0; i < WrapperConfig::MAX_MATCH_LIST && m.doors[i] != DOOR_COUNT; i++) {
                                if (w.door == m.doors[i]) { found = true; break; }
                            }
                            matched = found;
                        }
                        targetMatched = matched;
                    });
                });
            }
            if (!targetMatched) return;
        }

        // === Failure helper ===
        auto runOnFail = [&]() {
            for (int i = 0; i < WrapperConfig::MAX_EFFECTS; i++) {
                if (std::holds_alternative<NoEffect>(_config.effects.onFail[i])) break;
                std::visit([&](auto& effect) { effect.activate(activation); }, _config.effects.onFail[i]);
            }
        };

        // === Requirement phase (actor) ===

        const TraitBits missingActorTraits = _config.conditions.actor.required - computed;
        if (codeset.addFailure(missingActorTraits.isAny(), CODE_WRAPPER_MISSING_REQUIRED_TRAITS)) {
            for (int i = 0; i < TRAIT_COUNT; i++) {
                if (missingActorTraits[i].orElse(false)) {
                    controller.addRequestLoggedEvent(activation, LoggedEvent{
                        EVENT_MISSING_TRAIT,
                        { EventComponentKind::ROLE, (int)character.role },
                        {},
                        { EventComponentKind::TRAIT, i },
                        -1
                    });
                }
            }
            runOnFail();
            return;
        }

        const TraitBits restrictedActorTraits = _config.conditions.actor.restricted & computed;
        if (codeset.addFailure(restrictedActorTraits.isAny(), CODE_WRAPPER_HAS_RESTRICTED_TRAITS)) {
            for (int i = 0; i < TRAIT_COUNT; i++) {
                if (restrictedActorTraits[i].orElse(false)) {
                    controller.addRequestLoggedEvent(activation, LoggedEvent{
                        EVENT_RESTRICTED_TRAIT,
                        { EventComponentKind::ROLE, (int)character.role },
                        {},
                        { EventComponentKind::TRAIT, i },
                        -1
                    });
                }
            }
            runOnFail();
            return;
        }

        // === Requirement phase (tool) ===

        if (_config.conditions.tool.required.isAny() || _config.conditions.tool.restricted.isAny()) {
            activation.sourceItem.access([&](Item& item) {
                item.accessFlyweight([&](const ItemFlyweight& fw) {
                    const TraitBits missingToolTraits = _config.conditions.tool.required - fw.itemAttributes;
                    if (codeset.addFailure(missingToolTraits.isAny(), CODE_WRAPPER_MISSING_TOOL_TRAITS)) {
                        for (int i = 0; i < TRAIT_COUNT; i++) {
                            if (missingToolTraits[i].orElse(false)) {
                                controller.addRequestLoggedEvent(activation, LoggedEvent{
                                    EVENT_MISSING_TRAIT,
                                    { EventComponentKind::ROLE, (int)character.role },
                                    { EventComponentKind::ITEM, (int)item.type },
                                    { EventComponentKind::TRAIT, i },
                                    -1
                                });
                            }
                        }
                        runOnFail();
                        return;
                    }

                    const TraitBits restrictedToolTraits = _config.conditions.tool.restricted & fw.itemAttributes;
                    if (codeset.addFailure(restrictedToolTraits.isAny(), CODE_WRAPPER_HAS_RESTRICTED_TOOL_TRAITS)) {
                        for (int i = 0; i < TRAIT_COUNT; i++) {
                            if (restrictedToolTraits[i].orElse(false)) {
                                controller.addRequestLoggedEvent(activation, LoggedEvent{
                                    EVENT_RESTRICTED_TRAIT,
                                    { EventComponentKind::ROLE, (int)character.role },
                                    { EventComponentKind::ITEM, (int)item.type },
                                    { EventComponentKind::TRAIT, i },
                                    -1
                                });
                            }
                        }
                        runOnFail();
                    }
                });
            });
            if (codeset.isAnyFailure) return;
        }

        // === Requirement phase (target) ===

        if (_config.conditions.target.required.isAny() || _config.conditions.target.restricted.isAny()) {
            bool targetFound = false;
            activation.targetCharacter().access([&](Character& target) {
                targetFound = true;
                const auto targetComputed = controller.getTraitsComputed(target.characterId).final;

                const TraitBits missingTargetTraits = _config.conditions.target.required - targetComputed;
                if (codeset.addFailure(missingTargetTraits.isAny(), CODE_WRAPPER_MISSING_TARGET_TRAITS)) {
                    for (int i = 0; i < TRAIT_COUNT; i++) {
                        if (missingTargetTraits[i].orElse(false)) {
                            controller.addRequestLoggedEvent(activation, LoggedEvent{
                                EVENT_MISSING_TRAIT,
                                { EventComponentKind::ROLE, (int)character.role },
                                { EventComponentKind::ROLE, (int)target.role },
                                { EventComponentKind::TRAIT, i },
                                -1
                            });
                        }
                    }
                    runOnFail();
                    return;
                }

                const TraitBits restrictedTargetTraits = _config.conditions.target.restricted & targetComputed;
                if (codeset.addFailure(restrictedTargetTraits.isAny(), CODE_WRAPPER_HAS_RESTRICTED_TARGET_TRAITS)) {
                    for (int i = 0; i < TRAIT_COUNT; i++) {
                        if (restrictedTargetTraits[i].orElse(false)) {
                            controller.addRequestLoggedEvent(activation, LoggedEvent{
                                EVENT_RESTRICTED_TRAIT,
                                { EventComponentKind::ROLE, (int)character.role },
                                { EventComponentKind::ROLE, (int)target.role },
                                { EventComponentKind::TRAIT, i },
                                -1
                            });
                        }
                    }
                    runOnFail();
                }
            });
            if (!targetFound) {
                activation.targetItem().access([&](Item& item) {
                    targetFound = true;
                    item.accessFlyweight([&](const ItemFlyweight& fw) {
                        const TraitBits missingTargetTraits = _config.conditions.target.required - fw.itemAttributes;
                        if (codeset.addFailure(missingTargetTraits.isAny(), CODE_WRAPPER_MISSING_TARGET_TRAITS)) {
                            for (int i = 0; i < TRAIT_COUNT; i++) {
                                if (missingTargetTraits[i].orElse(false)) {
                                    controller.addRequestLoggedEvent(activation, LoggedEvent{
                                        EVENT_MISSING_TRAIT,
                                        { EventComponentKind::ROLE, (int)character.role },
                                        { EventComponentKind::ITEM, (int)item.type },
                                        { EventComponentKind::TRAIT, i },
                                        -1
                                    });
                                }
                            }
                            runOnFail();
                            return;
                        }

                        const TraitBits restrictedTargetTraits = _config.conditions.target.restricted & fw.itemAttributes;
                        if (codeset.addFailure(restrictedTargetTraits.isAny(), CODE_WRAPPER_HAS_RESTRICTED_TARGET_TRAITS)) {
                            for (int i = 0; i < TRAIT_COUNT; i++) {
                                if (restrictedTargetTraits[i].orElse(false)) {
                                    controller.addRequestLoggedEvent(activation, LoggedEvent{
                                        EVENT_RESTRICTED_TRAIT,
                                        { EventComponentKind::ROLE, (int)character.role },
                                        { EventComponentKind::ITEM, (int)item.type },
                                        { EventComponentKind::TRAIT, i },
                                        -1
                                    });
                                }
                            }
                            runOnFail();
                        }
                    });
                });
            }
            if (!targetFound) {
                activation.targetWall().access([&](Wall& w) {
                    DoorFlyweight::getFlyweights().accessConst((int)w.door, [&](const DoorFlyweight& fw) {
                        const TraitBits missingTargetTraits = _config.conditions.target.required - fw.doorAttributes;
                        if (codeset.addFailure(missingTargetTraits.isAny(), CODE_WRAPPER_MISSING_TARGET_TRAITS)) {
                            for (int i = 0; i < TRAIT_COUNT; i++) {
                                if (missingTargetTraits[i].orElse(false)) {
                                    controller.addRequestLoggedEvent(activation, LoggedEvent{
                                        EVENT_MISSING_TRAIT,
                                        { EventComponentKind::ROLE, (int)character.role },
                                        { EventComponentKind::DOOR, (int)w.door },
                                        { EventComponentKind::TRAIT, i },
                                        -1
                                    });
                                }
                            }
                            runOnFail();
                            return;
                        }

                        const TraitBits restrictedTargetTraits = _config.conditions.target.restricted & fw.doorAttributes;
                        if (codeset.addFailure(restrictedTargetTraits.isAny(), CODE_WRAPPER_HAS_RESTRICTED_TARGET_TRAITS)) {
                            for (int i = 0; i < TRAIT_COUNT; i++) {
                                if (restrictedTargetTraits[i].orElse(false)) {
                                    controller.addRequestLoggedEvent(activation, LoggedEvent{
                                        EVENT_RESTRICTED_TRAIT,
                                        { EventComponentKind::ROLE, (int)character.role },
                                        { EventComponentKind::DOOR, (int)w.door },
                                        { EventComponentKind::TRAIT, i },
                                        -1
                                    });
                                }
                            }
                            runOnFail();
                        }
                    });
                });
            }
            if (codeset.isAnyFailure) return;
        }

        // === Cost phase ===

        // If the actor has TRAIT_OBJECT (e.g. a sacrament or interactive object), redirect
        // action/move costs to the target character instead of the actor.
        Character* costPayer = &character;
        if (computed[(int)TRAIT_OBJECT].orElse(false)) {
            activation.targetCharacter().access([&](Character& target) {
                costPayer = &target;
            });
        }

        for (int i = 0; i < WrapperConfig::MAX_COSTS; i++) {
            if (_config.costs.item[i] == ITEM_NIL) break;
            if (codeset.addFailure(!controller.takeInventoryItem(inventory, _config.costs.item[i], true), CODE_WRAPPER_INSUFFICIENT_ITEM_COST)) {
                controller.addRequestLoggedEvent(activation, LoggedEvent{
                    EVENT_MISSING_ITEM,
                    { EventComponentKind::ROLE, (int)character.role },
                    {},
                    { EventComponentKind::ITEM, (int)_config.costs.item[i] },
                    -1
                });
                return;
            }
        }

        for (int i = 0; i < WrapperConfig::MAX_COSTS; i++) {
            if (_config.costs.item[i] == ITEM_NIL) break;
            if (codeset.addFailure(!controller.takeInventoryItem(inventory, _config.costs.item[i], req.time, req.room.roomId, req.isSkippingAnimations), CODE_WRAPPER_FAILED_TO_TAKE_ITEM)) return;
        }

        for (int i = 0; i < _config.costs.action; i++) {
            if (codeset.addFailure(!controller.takeCharacterAction(*costPayer), CODE_WRAPPER_INSUFFICIENT_ACTIONS)) {
                controller.addRequestLoggedEvent(activation, LoggedEvent{
                    EVENT_NO_ACTIONS,
                    { EventComponentKind::ROLE, (int)costPayer->role },
                    {},
                    {},
                    -1
                });
                return;
            }
        }

        for (int i = 0; i < _config.costs.move; i++) {
            if (codeset.addFailure(!controller.takeCharacterMove(*costPayer), CODE_WRAPPER_INSUFFICIENT_MOVES)) {
                controller.addRequestLoggedEvent(activation, LoggedEvent{
                    EVENT_NO_MOVES,
                    { EventComponentKind::ROLE, (int)costPayer->role },
                    {},
                    {},
                    -1
                });
                return;
            }
        }

        // === Success effects phase ===

        for (int i = 0; i < WrapperConfig::MAX_EFFECTS; i++) {
            if (std::holds_alternative<NoEffect>(_config.effects.onSuccess[i])) break;
            bool effectResult = std::visit([&](auto& effect) {
                return effect.activate(activation);
            }, _config.effects.onSuccess[i]);
            if (codeset.addFailure(!effectResult, CODE_WRAPPER_EFFECT_FAILED)) return;
        }
        result = true;
    });
    return result;
}
