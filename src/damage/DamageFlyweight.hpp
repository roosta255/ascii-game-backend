#pragma once

#include "Array.hpp"
#include "DamageAttrBits.hpp"
#include "DamageTypeBits.hpp"
#include "Pointer.hpp"
#include "TraitEnum.hpp"

class iActivator;

struct DamageFlyweight {
    DamageAttrBits attrs;
    TraitEnum resistTrait = TRAIT_COUNT;    // TRAIT_COUNT = no resistance
    TraitEnum weaknessTrait = TRAIT_COUNT;  // TRAIT_COUNT = no weakness
    Pointer<const iActivator> activator;

    static const Array<DamageFlyweight, DAMAGE_TYPE_COUNT>& getFlyweights();
    static const DamageTypeBits& getSingletonDamageVectors();
    static const DamageTypeBits& getSingletonDamageEffects();
};
