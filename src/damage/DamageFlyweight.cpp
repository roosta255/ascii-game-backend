#include "ActivatorDamageBite.hpp"
#include "ActivatorDamageCold.hpp"
#include "ActivatorDamageCrush.hpp"
#include "ActivatorDamageElectric.hpp"
#include "ActivatorDamageFire.hpp"
#include "ActivatorDamagePierce.hpp"
#include "DamageFlyweight.hpp"
#include "DamageTypeEnum.hpp"
#include "iActivator.hpp"
#include "TraitEnum.hpp"

const Array<DamageFlyweight, DAMAGE_TYPE_COUNT>& DamageFlyweight::getFlyweights() {
    static auto flyweights = [](){
        Array<DamageFlyweight, DAMAGE_TYPE_COUNT> flyweights;
        DamageTypeEnum lastType = DAMAGE_TYPE_MELEE;

        #define DAMAGE_TYPE_DECL(name_, attrs_, activator_) \
            static activator_ GLOBAL_DAMAGE_##name_##_activator_; \
            lastType = DAMAGE_TYPE_##name_; \
            flyweights.getPointer(lastType).access([&](DamageFlyweight& fw){ \
                fw.attrs = makeDamageAttrBits attrs_; \
                fw.activator = GLOBAL_DAMAGE_##name_##_activator_; \
            });
        #define DAMAGE_TYPE_RESIST_DECL(trait_) \
            flyweights.getPointer(lastType).access([&](DamageFlyweight& fw){ \
                fw.resistTrait = trait_; \
            });
        #define DAMAGE_TYPE_WEAKNESS_DECL(trait_) \
            flyweights.getPointer(lastType).access([&](DamageFlyweight& fw){ \
                fw.weaknessTrait = trait_; \
            });

        #include "DamageType.enum"

        #undef DAMAGE_TYPE_DECL
        #undef DAMAGE_TYPE_RESIST_DECL
        #undef DAMAGE_TYPE_WEAKNESS_DECL

        return flyweights;
    }();
    return flyweights;
}

const DamageTypeBits& DamageFlyweight::getSingletonDamageVectors() {
    static auto bits = [](){
        DamageTypeBits result;
        const auto& fws = getFlyweights();
        for (int i = 0; i < (int)DAMAGE_TYPE_COUNT; i++) {
            fws.accessConst(i, [&](const DamageFlyweight& fw){
                if (fw.attrs[DAMAGE_ATTR_VECTOR].orElse(false))
                    result.setIndexOn(i);
            });
        }
        return result;
    }();
    return bits;
}

const DamageTypeBits& DamageFlyweight::getSingletonDamageEffects() {
    static auto bits = [](){
        DamageTypeBits result;
        const auto& fws = getFlyweights();
        for (int i = 0; i < (int)DAMAGE_TYPE_COUNT; i++) {
            fws.accessConst(i, [&](const DamageFlyweight& fw){
                if (fw.attrs[DAMAGE_ATTR_EFFECT].orElse(false))
                    result.setIndexOn(i);
            });
        }
        return result;
    }();
    return bits;
}
