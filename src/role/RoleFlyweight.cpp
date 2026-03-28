#include "ActivatorWrapper.hpp"
#include "Array.hpp"
#include "DamageTypeBits.hpp"
#include "RoleEnum.hpp"
#include "RoleFlyweight.hpp"
#include "Match.hpp"
#include "iActivator.hpp"
#include "ActivatorToggler.hpp"

const Array<RoleFlyweight, ROLE_COUNT>& RoleFlyweight::getFlyweights()
{
    static auto flyweights = [](){
        Array<RoleFlyweight, ROLE_COUNT> flyweights;
        RoleEnum lastRole = ROLE_EMPTY;

        static Array<WrapperConfig, ROLE_COUNT> useWrapperConfigs;
        static Array<ActivatorWrapper, ROLE_COUNT> useWrappers;

        #define ROLE_DECL( name_, health_, moves_, actions_, feats_, activator_, traits_sourced_ ) \
            static activator_ GLOBAL_##name_##activator_; \
            lastRole = ROLE_##name_; \
            flyweights.getPointer( lastRole ).access([&](RoleFlyweight& flyweight){ \
                flyweight.health = health_; \
                flyweight.name = #name_; \
                flyweight.isMovable = (moves_ > 0); \
                flyweight.moves = moves_; \
                flyweight.isActionable = !std::is_same_v<activator_, iActivator>; \
                flyweight.actions = actions_; \
                flyweight.feats = feats_; \
                flyweight.activator = GLOBAL_##name_##activator_; \
                flyweight.traitsSourced = makeTraitBits traits_sourced_ ; \
            });
        #define ROLE_BITE_MODIFIER_DECL( bite_trait_modifier_ ) \
            flyweights.getPointer( lastRole ).access([&](RoleFlyweight& flyweight){ \
                flyweight.biteTraitModifier = TraitModifier bite_trait_modifier_ ; \
            });
        #define ROLE_DAMAGE_TYPES_DECL( damage_types_ ) \
            flyweights.getPointer( lastRole ).access([&](RoleFlyweight& flyweight){ \
                flyweight.damageTypes = makeDamageTypeBits damage_types_ ; \
            });
        #define ROLE_DEATH_DECL( activator_ ) \
            static activator_ GLOBAL_##activator_##death_; \
            flyweights.getPointer( lastRole ).access([&](RoleFlyweight& flyweight){ \
                flyweight.deathActivator = GLOBAL_##activator_##death_; \
            });
        #define ROLE_DAMAGE_DECL( activator_ ) \
            static activator_ GLOBAL_##activator_##damage_; \
            flyweights.getPointer( lastRole ).access([&](RoleFlyweight& flyweight){ \
                flyweight.damageActivator = GLOBAL_##activator_##damage_; \
            });
        #define ROLE_ATTACK_DECL( activator_ ) \
            static activator_ GLOBAL_##activator_##attack_; \
            flyweights.getPointer( lastRole ).access([&](RoleFlyweight& flyweight){ \
                flyweight.attackActivator = GLOBAL_##activator_##attack_; \
            });
        #define ROLE_USE_WRAPPER(...) \
            useWrapperConfigs.access((int)lastRole, [](WrapperConfig& config) { \
                config = WrapperConfig __VA_ARGS__; \
            });

        #include "Role.enum"
        #undef ROLE_DECL
        #undef ROLE_BITE_MODIFIER_DECL
        #undef ROLE_DAMAGE_TYPES_DECL
        #undef ROLE_DEATH_DECL
        #undef ROLE_DAMAGE_DECL
        #undef ROLE_ATTACK_DECL
        #undef ROLE_USE_WRAPPER

        for (int i = 0; i < ROLE_COUNT; i++) {
            useWrapperConfigs.access(i, [&](WrapperConfig& config) {
                if (!config.isEmpty()) {
                    useWrappers.access(i, [&](ActivatorWrapper& wrapper) {
                        wrapper.init(config);
                    });
                    flyweights.access(i, [&](RoleFlyweight& flyweight) {
                        useWrappers.access(i, [&](ActivatorWrapper& wrapper) {
                            flyweight.activator = wrapper;
                            flyweight.isActionable = true;
                        });
                    });
                }
            });
        }

        return flyweights;
    }();
    return flyweights;
}

bool RoleFlyweight::indexByString
(
    const std::string& name,
    RoleEnum& output
) {
    int i = 0;
    const auto& flyweights = getFlyweights(); 
    for(const RoleFlyweight& flyweight: flyweights) {
        if(name == flyweight.name) {
            output = (RoleEnum)i;
            return true;
        }
        i++;
    }
    return false;
}
