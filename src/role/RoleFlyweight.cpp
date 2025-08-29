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

        #define ROLE_DECL( name_, health_, foggy_, moves_, actions_, feats_, activator_, isActor_, isKeyer_, isObject_ ) \
            static activator_ GLOBAL_##name_##activator_; \
            flyweights.getPointer( ROLE_##name_ ).access([&](RoleFlyweight& flyweight){ \
                flyweight.health = health_; \
                flyweight.name = #name_; \
                flyweight.foggy = foggy_; \
                flyweight.isMovable = (moves_ > 0); \
                flyweight.moves = moves_; \
                flyweight.isActionable = !std::is_same_v<activator_, iActivator>; \
                flyweight.actions = actions_; \
                flyweight.feats = feats_; \
                flyweight.activator = GLOBAL_##name_##activator_; \
                flyweight.isActor = isActor_; \
                flyweight.isKeyer = isKeyer_; \
                flyweight.isObject = isObject_; \
            });
        #include "Role.enum"
        #undef ROLE_DECL

        return flyweights;
    }();
    return flyweights;
}

bool RoleFlyweight::indexByString
(
    const std::string& name,
    int& output
) {
    output = 0;
    const auto& flyweights = getFlyweights(); 
    for(const RoleFlyweight& flyweight: flyweights) {
        if(name == flyweight.name) {
            return true;
        }
        output++;
    }
    return false;
}
