#include "ActivatorAttack.hpp"
#include "Activation.hpp"
#include "ActivatorDamage.hpp"
#include "Character.hpp"
#include "Codeset.hpp"
#include "DamageTypeBits.hpp"
#include "DamageTypeEnum.hpp"
#include "Inventory.hpp"
#include "ItemFlyweight.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "RoleFlyweight.hpp"
#include "TraitEnum.hpp"

bool ActivatorAttack::activate(Activation& activation) const {
    auto& codeset = activation.request->codeset;
    auto& controller = activation.request->controller;
    auto& attacker = activation.character;

    bool isSuccess = false;
    const bool isTargetAccessed = activation.target.access([&](Character& target) {
        const auto targetTraits = controller.getTraitsComputed(target.characterId);

        // --- PARRY check ---
        // If target has PARRY and both ACTION_READY and MOVEMENT_READY,
        // cancel the attack and have the target counter-attack the attacker.
        const bool hasParry = targetTraits.final[TRAIT_PARRY].orElse(false);
        const bool isActionReady = targetTraits.final[TRAIT_ACTION_READY].orElse(false);
        const bool isMovementReady = targetTraits.final[TRAIT_MOVEMENT_READY].orElse(false);
        if (hasParry && isActionReady && isMovementReady) {
            codeset.addTable(CODE_DAMAGE_PARRY_TRIGGERED, true);
            if (codeset.addFailure(!controller.takeCharacterAction(target), CODE_DAMAGE_FAILED_TO_PARRY_TAKE_ACTION)) {
                return;
            }
            if (codeset.addFailure(!controller.takeCharacterMove(target), CODE_DAMAGE_FAILED_TO_PARRY_TAKE_MOVE)) {
                return;
            }
            // Counter-attack: target attacks attacker (swap roles)
            ActivationContext counterActivation = {
                .request = activation.request,
                .character = target,
                .target = Pointer<Character>(attacker),
                .sourceItem = activation.sourceItem,
                .targetItem = activation.targetItem,
                .sourceInventory = activation.sourceInventory,
                .targetInventory = activation.targetInventory,
                .direction = activation.direction,
                .damageTypes = activation.damageTypes
            };
            static ActivatorAttack innerAttack;
            innerAttack.activate(counterActivation);
            isSuccess = true;
            return;
        }

        // --- Determine damage types ---
        // Start with effect types from the item or role.
        // Then add DAMAGE_TYPE_MELEE as the attack vector.
        DamageTypeBits damageTypes;
        bool hasItemDamage = false;
        activation.sourceItem.accessConst([&](const Item& item) {
            CodeEnum err = CODE_UNKNOWN_ERROR;
            item.accessFlyweight([&](const ItemFlyweight& flyweight) {
                if (flyweight.damageTypes.isAny()) {
                    damageTypes = flyweight.damageTypes;
                    hasItemDamage = true;
                }
            });
        });

        if (!hasItemDamage) {
            CodeEnum err = CODE_UNKNOWN_ERROR;
            attacker.accessRole(err, [&](const RoleFlyweight& role) {
                damageTypes = role.damageTypes;
            });
        }

        // All attacks via this activator are melee.
        damageTypes.setIndexOn(DAMAGE_TYPE_MELEE);

        // --- Activate damage process on target ---
        ActivationContext damageActivation = {
            .request = activation.request,
            .character = attacker,
            .target = Pointer<Character>(target),
            .sourceItem = activation.sourceItem,
            .targetItem = activation.targetItem,
            .sourceInventory = activation.sourceInventory,
            .targetInventory = activation.targetInventory,
            .direction = activation.direction,
            .damageTypes = damageTypes
        };

        static ActivatorDamage damageActivator;
        if (codeset.addFailure(!damageActivator.activate(damageActivation), CODE_ATTACK_FAILED_TO_ACTIVATE_DAMAGE)) {
            return;
        }

        isSuccess = true;
    });

    codeset.addFailure(!isTargetAccessed, CODE_ATTACK_MISSING_TARGET);
    return isTargetAccessed && isSuccess;
}
