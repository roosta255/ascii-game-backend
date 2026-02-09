#include "ActivatorUseCharacter.hpp"
#include "Codeset.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "RoleEnum.hpp"
#include "RoleFlyweight.hpp"

bool ActivatorUseCharacter::activate(Activation& activation) const {
    auto& controller = activation.controller;
    auto& codeset = activation.codeset;
    auto& match = activation.match;
    auto& player = activation.player;
    auto& inventory = player.inventory;
    auto& room = activation.room;
    auto& subject = activation.character;
    auto& object = activation.target;

    codeset.setTable(CODE_IS_BLUE_OPEN_BEFORE, match.dungeon.isBlueOpen ? 1 : -1);
    codeset.setTable(CODE_SUBJECT_CHARACTER_ROLE, subject.role);
    if (subject.role == ROLE_TOGGLER) {
        codeset.setTable(CODE_SUBJECT_TOGGLER_CHARACTER_ID, subject.characterId);
    }
    codeset.setTable(CODE_ACTIVATION_SUBJECT_CHARACTER_ID, subject.characterId);
    object.accessConst([&](const Character& targetCharacter){
        codeset.setTable(CODE_ACTIVATION_OBJECT_CHARACTER_ID, targetCharacter.characterId);
        if (targetCharacter.role == ROLE_TOGGLER) {
            codeset.setTable(CODE_OBJECT_TOGGLER_CHARACTER_ID, targetCharacter.characterId);
        }
        if (targetCharacter.role == ROLE_BUILDER) {
            codeset.setTable(CODE_OBJECT_BUILDER_CHARACTER_ID, targetCharacter.characterId);
        }
        codeset.setTable(CODE_OBJECT_CHARACTER_ROLE, targetCharacter.role);
    });

    // Check if character can take an action
    if (codeset.addFailure(!subject.isActionable(codeset.error))) {
        return false;
    }

    if (codeset.addFailure(!controller.validateCharacterWithinRoom(subject.characterId, room.roomId), CODE_SUBJECT_CHARACTER_NOT_IN_ROOM)) {
        return false;
    }

    bool isTargetValid = false;
    const bool isTargetSpecified = object.access([&](Character& target) {
        if (codeset.addFailure(!controller.validateCharacterWithinRoom(target.characterId, room.roomId), CODE_TARGET_CHARACTER_NOT_IN_ROOM)) {
            return;
        }
        isTargetValid = true;
    });

    if (!isTargetSpecified) {
        codeset.addLog(CODE_ACTIVATION_TARGET_NOT_SPECIFIED);
        isTargetValid = true;
    }

    if (codeset.addFailure(!isTargetValid, CODE_ACTIVATION_INVALID_TARGET)) {
        return false;
    }

    bool isSuccess = false;
    codeset.addFailure(!subject.accessRole(codeset.error, [&](const RoleFlyweight& flyweight) {
        if (codeset.addFailure(flyweight.activator.isEmpty(), CODE_MISSING_ACTIVATOR)) {
            return;
        } else {
            flyweight.activator.accessConst([&](const iActivator& activatorIntf){
                codeset.addFailure(!(isSuccess = activatorIntf.activate(activation)));
            });
        }
    }), CODE_INACCESSIBLE_ROLE_FLYWEIGHT);
    codeset.setTable(CODE_IS_BLUE_OPEN_AFTER, match.dungeon.isBlueOpen ? 1 : -1);
    return isSuccess;
}