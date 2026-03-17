#include "ActivatorCritterBite.hpp"
#include "Character.hpp"
#include "Codeset.hpp"
#include "Keyframe.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Rack.hpp"
#include "RoleFlyweight.hpp"
#include "TraitEnum.hpp"
#include "TraitModifier.hpp"

bool ActivatorCritterBite::activate(Activation& activation) const {
    auto& codeset = activation.codeset;
    auto& subject = activation.character;

    bool isBiting = activation.controller.getTraitsComputed(subject.characterId).final[TRAIT_CRITTER_BITES].orElse(false);
    codeset.addTable(CODE_CRITTER_BITE_ACTIVATED_WITH_BITES_TRAIT, isBiting);
    codeset.addTable(CODE_CRITTER_BITE_ACTIVATED_WITHOUT_BITES_TRAIT, !isBiting);
    if (!isBiting) {
        return true;
    }

    // subject is the critter — look up its bite modifier from the role flyweight
    // target is the character being bitten
    bool isSuccess = false;
    const bool isTargetAccessed = activation.target.access([&](Character& target) {
        CodeEnum roleError = CODE_UNKNOWN_ERROR;
        subject.accessRole(roleError, [&](const RoleFlyweight& critterRole) {
            critterRole.biteTraitModifier.accessConst([&](const TraitModifier& modifier) {
                modifier.applyAffliction(target);
                activation.controller.updateTraits(target);
                isSuccess = true;
            });
        });

        if (isSuccess && !activation.isSkippingAnimations) {
            const int roomId = activation.room.roomId;
            Keyframe::insertKeyframe(
                Rack<Keyframe>::buildFromArray<Character::MAX_KEYFRAMES>(subject.keyframes),
                Keyframe::buildJump(activation.time, 1800, roomId)
            );
            Keyframe::insertKeyframe(
                Rack<Keyframe>::buildFromArray<Character::MAX_KEYFRAMES>(target.keyframes),
                Keyframe::buildCritterBite(activation.time, 1800, roomId)
            );
        }
    });

    codeset.addFailure(!isTargetAccessed, CODE_CRITTER_BITE_MISSING_TARGET);

    return isSuccess;
}
