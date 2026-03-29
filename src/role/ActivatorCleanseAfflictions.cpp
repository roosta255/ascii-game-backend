#include "ActivatorCleanseAfflictions.hpp"
#include "Character.hpp"
#include "TraitEnum.hpp"
#include "TraitFlyweight.hpp"
#include "TraitType.hpp"

bool ActivatorCleanseAfflictions::activate(Activation& activation) const {
    auto& character = activation.character;
    const auto& flyweights = TraitFlyweight::getFlyweights();

    for (int i = 0; i < TRAIT_COUNT; i++) {
        character.traitsAfflicted[i].access([&](const bool isSet) {
            if (!isSet) return;
            flyweights.accessConst(i, [&](const TraitFlyweight& flyweight) {
                flyweight.types[TRAIT_TYPE_AFFLICTION].access([&](const bool isAffliction) {
                    if (isAffliction) character.traitsAfflicted.setIndexOff(i);
                });
            });
        });
    }

    return true;
}
