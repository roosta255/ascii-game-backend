#include "ActionFlyweight.hpp"
#include "ActivatorEndTurn.hpp"
#include "ActivatorMoveToDoor.hpp"
#include "ActivatorMoveToFloor.hpp"
#include "ActivatorUseCharacter.hpp"
#include "ActivatorUseDoor.hpp"
#include "ActivatorUseInventoryItem.hpp"
#include "ActivatorUseLock.hpp"
#include "Array.hpp"
#include "Bitcard.hpp"
#include "Match.hpp"
#include "iActivator.hpp"
#include "ActivatorToggler.hpp"
#include <unordered_map>

const Array<ActionFlyweight, ACTION_COUNT>& ActionFlyweight::getFlyweights()
{
    static auto flyweights = [](){
        Array<ActionFlyweight, ACTION_COUNT> flyweights;

        #define ACTION_DECL( name_, activation_intf_ ) \
            static activation_intf_ GLOBAL_##name_##activation_intf_; \
            flyweights.getPointer( ACTION_##name_ ).access([&](ActionFlyweight& flyweight){ \
                flyweight.name = #name_; \
                flyweight.activator = GLOBAL_##name_##activation_intf_; \
            });
        #include "Action.enum"
        #undef ACTION_DECL

        return flyweights;
    }();
    return flyweights;
}

const iActivator& ActionFlyweight::getMoveToDoor() {
    static ActivatorMoveToDoor activator;
    return activator;
}

const iActivator& ActionFlyweight::getMoveToFloor() {
    static ActivatorMoveToFloor activator;
    return activator;
}

const iActivator& ActionFlyweight::getUseCharacter() {
    static ActivatorUseCharacter activator;
    return activator;
}

const iActivator& ActionFlyweight::getUseDoor() {
    static ActivatorUseDoor activator;
    return activator;
}

const iActivator& ActionFlyweight::getUseInventoryItem() {
    static ActivatorUseInventoryItem activator;
    return activator;
}

const iActivator& ActionFlyweight::getUseLock() {
    static ActivatorUseLock activator;
    return activator;
}

bool ActionFlyweight::indexByString
(
    const std::string& name,
    ActionEnum& output
) {
    static const auto nameMap = [](){
        std::unordered_map<std::string, ActionEnum> nameMap;
        const auto& flyweights = getFlyweights();
        int i = 0;
        for(const ActionFlyweight& flyweight: flyweights) {
            nameMap[flyweight.name] = ActionEnum(i++);
        }
        return nameMap;
    }();
    if (auto search = nameMap.find(name); search != nameMap.end()) {
        output = search->second;
        return true;
    }
    return false;
}
