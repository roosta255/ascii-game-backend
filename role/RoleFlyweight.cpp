#include "Array.hpp"
#include "RoleEnum.hpp"
#include "RoleFlyweight.hpp"

const Array<RoleFlyweight, ROLE_COUNT>& RoleFlyweight::getFlyweights()
{
    static auto flyweights = [](){

        Array<RoleFlyweight, ROLE_COUNT> flyweights;
        #define ROLE_DECL( name_, health_, foggy_, isMovable_, moves_, isActionable_, actions_, feats_ ) flyweights.getPointer( ROLE_##name_ ).access([](RoleFlyweight& flyweight){ flyweight.health = health_; flyweight.name = #name_; flyweight.foggy = foggy_; flyweight.isMovable = isMovable_; flyweight.moves = moves_; flyweight.isActionable = isActionable_; flyweight.actions = actions_; flyweight.feats = feats_; });
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
