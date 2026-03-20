#include "ActivatorDamage.hpp"
#include "Activation.hpp"
#include "Character.hpp"
#include "Codeset.hpp"
#include "DamageFlyweight.hpp"
#include "DamageTypeEnum.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "TraitEnum.hpp"

bool ActivatorDamage::activate(Activation& activation) const {
    auto& codeset = activation.codeset;
    auto& controller = activation.controller;

    bool isSuccess = false;
    const bool isTargetAccessed = activation.target.access([&](Character& target) {
        const auto traitsComputed = controller.getTraitsComputed(target.characterId);
        const auto& flyweights = DamageFlyweight::getFlyweights();
        const auto& damageVectors = DamageFlyweight::getSingletonDamageVectors();
        const auto& damageEffects = DamageFlyweight::getSingletonDamageEffects();

        // --- Vector resistance check ---
        // If active vectors exist and ALL of them are resisted, cancel all damage.
        const auto activeVectors = activation.damageTypes & damageVectors;
        if (activeVectors.isAny()) {
            bool allVectorsResisted = true;
            for (int i = 0; i < (int)DAMAGE_TYPE_COUNT; i++) {
                if (!activeVectors[i].orElse(false)) continue;
                flyweights.accessConst(i, [&](const DamageFlyweight& fw) {
                    const bool resisted = (fw.resistTrait != TRAIT_COUNT)
                        && traitsComputed.final[fw.resistTrait].orElse(false);
                    if (!resisted) allVectorsResisted = false;
                });
            }
            codeset.addTable(CODE_DAMAGE_ALL_VECTORS_RESISTED, allVectorsResisted);
            if (allVectorsResisted) { isSuccess = true; return; }
        }

        // --- Effect resistance check ---
        // If all active effect types are resisted, base damage = 0.
        const auto activeEffects = activation.damageTypes & damageEffects;
        bool allEffectsResisted = activeEffects.isAny();
        for (int i = 0; i < (int)DAMAGE_TYPE_COUNT; i++) {
            if (!activeEffects[i].orElse(false)) continue;
            flyweights.accessConst(i, [&](const DamageFlyweight& fw) {
                const bool resisted = (fw.resistTrait != TRAIT_COUNT)
                    && traitsComputed.final[fw.resistTrait].orElse(false);
                if (!resisted) allEffectsResisted = false;
            });
        }
        codeset.addTable(CODE_DAMAGE_ALL_TYPES_RESISTED, allEffectsResisted);

        // --- Base damage ---
        int baseDamage = allEffectsResisted ? 0 : 1;

        // POISE: if target has poise and is action-ready, negate base damage.
        if (baseDamage > 0) {
            const bool hasPoise = traitsComputed.final[TRAIT_POISE].orElse(false);
            const bool isActionReady = traitsComputed.final[TRAIT_ACTION_READY].orElse(false);
            if (hasPoise && isActionReady) {
                baseDamage = 0;
                codeset.addTable(CODE_DAMAGE_POISE_NEGATED_BASE, true);
            }
        }

        target.damage += baseDamage;

        // --- Per-effect activators (called only for non-resisted types) ---
        for (int i = 0; i < (int)DAMAGE_TYPE_COUNT; i++) {
            if (!activeEffects[i].orElse(false)) continue;
            flyweights.accessConst(i, [&](const DamageFlyweight& fw) {
                const bool resisted = (fw.resistTrait != TRAIT_COUNT)
                    && traitsComputed.final[fw.resistTrait].orElse(false);
                if (resisted) return;
                fw.activator.accessConst([&](const iActivator& act) {
                    act.activate(activation);
                });
            });
        }

        isSuccess = true;
    });

    codeset.addFailure(!isTargetAccessed, CODE_DAMAGE_MISSING_TARGET);
    return isTargetAccessed && isSuccess;
}
