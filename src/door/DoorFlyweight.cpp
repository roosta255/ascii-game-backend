#include "Array.hpp"
#include "DoorEnum.hpp"
#include "DoorFlyweight.hpp"
#include "iActivator.hpp"
#include "ActivatorElevator.hpp"
#include "ActivatorJailer.hpp"
#include "ActivatorKeeper.hpp"
#include "ActivatorInactiveDoor.hpp"
#include "ActivatorWrapper.hpp"
#include "EventFlyweight.hpp"
#include "ActivatorBounceLock.cpp"
#include "ActivatorLadder.cpp"
#include "ActivatorLightningRod.cpp"
#include "ActivatorPole.cpp"
#include "ActivatorGiveItem.cpp"
#include "ActivatorSetDoor.cpp"
#include "ActivatorValidateNotOccupied.cpp"
#include "ActivatorSetSharedDoors.cpp"
#include "ActivatorShifter.cpp"
#include "ActivatorTimeGate.cpp"
#include "ActivatorTimeGateCube.cpp"
#include "ActivatorWrapper.cpp"

const Array<DoorFlyweight, DOOR_COUNT>& DoorFlyweight::getFlyweights() {
    static auto flyweights = [](){
        Array<DoorFlyweight, DOOR_COUNT> flyweights;
        DoorEnum lastDoor = DOOR_COUNT; // sentinel; set by DOOR_DECL before any config macros

        static Array<WrapperConfig, DOOR_COUNT> doorWrapperConfigs;
        static Array<WrapperConfig, DOOR_COUNT> lockWrapperConfigs;
        static Array<ActivatorWrapper, DOOR_COUNT> doorWrappers;
        static Array<ActivatorWrapper, DOOR_COUNT> lockWrappers;

        #define DOOR_DECL( name_text, blockingness, doorActivator_, lockActivator_, doorway_, is_shared_doorway_ ) \
            lastDoor = DOOR_##name_text; \
            static doorActivator_ GLOBAL_DOOR_##name_text##doorActivator_; \
            static lockActivator_ GLOBAL_LOCK_##name_text##lockActivator_; \
            flyweights.getPointer( DOOR_##name_text ).access([](DoorFlyweight& flyweight){ \
                flyweight.blocking = blockingness; \
                flyweight.name = #name_text; \
                flyweight.isDoorway = doorway_; \
                flyweight.isSharedDoorway = is_shared_doorway_; \
                flyweight.isDoorActionable = !std::is_same_v<doorActivator_, iActivator>; \
                flyweight.isLockActionable = !std::is_same_v<lockActivator_, iActivator>; \
                flyweight.doorActivator = GLOBAL_DOOR_##name_text##doorActivator_; \
                flyweight.lockActivator  = GLOBAL_LOCK_##name_text##lockActivator_; \
            });

        #define DOOR_LOCK_WRAPPER(...) \
            lockWrapperConfigs.access((int)lastDoor, [](WrapperConfig& config) { \
                config = WrapperConfig __VA_ARGS__; \
            });

        #define DOOR_DOOR_WRAPPER(...) \
            doorWrapperConfigs.access((int)lastDoor, [](WrapperConfig& config) { \
                config = WrapperConfig __VA_ARGS__; \
            });

        #include "Door.enum"
        #undef DOOR_DECL
        #undef DOOR_LOCK_WRAPPER
        #undef DOOR_DOOR_WRAPPER

        // Wrapper construction phase — builds wrappers from non-empty configs
        for (int i = 0; i < DOOR_COUNT; i++) {
            flyweights.access(i, [&](DoorFlyweight& flyweight) {
                doorWrapperConfigs.access(i, [&](WrapperConfig& config) {
                    if (!config.isEmpty()) {
                        doorWrappers.access(i, [&](ActivatorWrapper& wrapper) {
                            wrapper.init(config);
                        });
                        doorWrappers.access(i, [&](ActivatorWrapper& wrapper) {
                            flyweight.doorActivator = wrapper;
                            flyweight.isDoorActionable = true;
                        });
                    }
                });
                lockWrapperConfigs.access(i, [&](WrapperConfig& config) {
                    if (!config.isEmpty()) {
                        lockWrappers.access(i, [&](ActivatorWrapper& wrapper) {
                            wrapper.init(config);
                        });
                        lockWrappers.access(i, [&](ActivatorWrapper& wrapper) {
                            flyweight.lockActivator = wrapper;
                            flyweight.isLockActionable = true;
                        });
                    }
                });
            });
        }

        return flyweights;
    }();
    return flyweights;
}

bool DoorFlyweight::indexByString (const std::string& name, DoorEnum& output) {
    int i = 0;
    const auto& flyweights = getFlyweights();
    for(const DoorFlyweight& flyweight: flyweights) {
        if(name == flyweight.name) {
            output = (DoorEnum)i;
            return true;
        }
        i++;
    }
    return false;
}

std::ostream& operator<<(std::ostream& os, const DoorEnum& code) {
    DoorFlyweight::getFlyweights().accessConst(code, [&](const DoorFlyweight& flyweight){
        os << std::string(flyweight.name);
    });
    return os;
}
