#include "Array.hpp"
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

        #include "Role.enum"
        #undef ROLE_DECL
        #undef ROLE_BITE_MODIFIER_DECL

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
