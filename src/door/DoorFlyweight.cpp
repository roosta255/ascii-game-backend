#include "Array.hpp"
#include "DoorEnum.hpp"
#include "DoorFlyweight.hpp"
#include "iActivator.hpp"
#include "ActivatorJailer.hpp"
#include "ActivatorKeeper.hpp"
#include "ActivatorInactiveDoor.hpp"
#include "ActivatorLightningRod.cpp"
#include "ActivatorShifter.cpp"
#include "ActivatorTimeGate.cpp"
#include "ActivatorTimeGateCube.cpp"

const Array<DoorFlyweight, DOOR_COUNT>& DoorFlyweight::getFlyweights() {
    static auto flyweights = [](){
        Array<DoorFlyweight, DOOR_COUNT> flyweights;
        #define DOOR_DECL( name_text, blockingness, doorActivator_, lockActivator_, doorway_ ) \
            static doorActivator_ GLOBAL_DOOR_##name_text##doorActivator_; \
            static lockActivator_ GLOBAL_LOCK_##name_text##lockActivator_; \
            flyweights.getPointer( DOOR_##name_text ).access([](DoorFlyweight& flyweight){ \
                flyweight.blocking = blockingness; \
                flyweight.name = #name_text; \
                flyweight.isDoorway = doorway_; \
                flyweight.isDoorActionable = !std::is_same_v<doorActivator_, iActivator>; \
                flyweight.isLockActionable = !std::is_same_v<lockActivator_, iActivator>; \
                flyweight.doorActivator = GLOBAL_DOOR_##name_text##doorActivator_; \
                flyweight.lockActivator  = GLOBAL_LOCK_##name_text##lockActivator_; \
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
