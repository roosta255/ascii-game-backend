#include "Array.hpp"
#include "KeyframeFlyweight.hpp"

const Array<KeyframeFlyweight, ANIMATION_COUNT>& KeyframeFlyweight::getFlyweights()
{
    static auto flyweights = [](){
        Array<KeyframeFlyweight, ANIMATION_COUNT> flyweights;

        #define ANIMATION_DECL( name_ ) \
            flyweights.getPointer( ANIMATION_##name_ ).access([&](KeyframeFlyweight& flyweight){ \
                flyweight.name = #name_; \
            });
        #include "Animation.enum"
        #undef ANIMATION_DECL

        return flyweights;
    }();
    return flyweights;
}

bool KeyframeFlyweight::indexByString
(
    const std::string& name,
    int& output
) {
    output = 0;
    const auto& flyweights = getFlyweights(); 
    for(const KeyframeFlyweight& flyweight: flyweights) {
        if(name == flyweight.name) {
            return true;
        }
        output++;
    }
    return false;
}
