#include "Array.hpp"
#include "DoorEnum.hpp"
#include "DoorFlyweight.hpp"
#include "iActivator.hpp"
#include "ActivatorKeeper.hpp"
#include "ActivatorInactiveDoor.hpp"
#include "ActivatorShifter.hpp"

const Array<DoorFlyweight, DOOR_COUNT>& DoorFlyweight::getFlyweights() {
    static auto flyweights = [](){
        Array<DoorFlyweight, DOOR_COUNT> flyweights;
        #define DOOR_DECL( name_text, blockingness, activator_, doorway_ ) \
            static activator_ GLOBAL_##name_text##activator_; \
            flyweights.getPointer( DOOR_##name_text ).access([](DoorFlyweight& flyweight){ \
                flyweight.blocking = blockingness; \
                flyweight.name = #name_text; \
                flyweight.isDoorway = doorway_; \
                flyweight.activator = GLOBAL_##name_text##activator_; \
            });
        #include "Door.enum"
        #undef DOOR_DECL

        return flyweights;
    }();
    return flyweights;
}

bool DoorFlyweight::indexByString (const std::string& name, int& output) {
    output = 0;
    const auto& flyweights = getFlyweights(); 
    for(const DoorFlyweight& flyweight: flyweights) {
        if(name == flyweight.name) {
            return true;
        }
        output++;
    }
    return false;
}
