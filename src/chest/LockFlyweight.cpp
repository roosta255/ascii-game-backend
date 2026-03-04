#include "Array.hpp"
#include "LockFlyweight.hpp"
#include "Match.hpp"
#include "iActivator.hpp"
#include "ActivatorToggler.hpp"
#include <unordered_map>

const Array<LockFlyweight, LOCK_COUNT>& LockFlyweight::getFlyweights()
{
    static auto flyweights = [](){
        Array<LockFlyweight, LOCK_COUNT> flyweights;

        #define LOCK_DECL( name_, is_locked_, activation_intf_ ) \
            static activation_intf_ GLOBAL_##name_##activation_intf_; \
            flyweights.getPointer( LOCK_##name_ ).access([&](LockFlyweight& flyweight){ \
                flyweight.name = #name_; \
                flyweight.isLocked = is_locked_; \
                flyweight.activator = GLOBAL_##name_##activation_intf_; \
            });
        #include "Lock.enum"
        #undef LOCK_DECL

        return flyweights;
    }();
    return flyweights;
}

bool LockFlyweight::indexByString
(
    const std::string& name,
    LockEnum& output
) {
    static const auto nameMap = [](){
        std::unordered_map<std::string, LockEnum> nameMap;
        const auto& flyweights = getFlyweights();
        int i = 0;
        for(const LockFlyweight& flyweight: flyweights) {
            nameMap[flyweight.name] = LockEnum(i++);
        }
        return nameMap;
    }();
    if (auto search = nameMap.find(name); search != nameMap.end()) {
        output = search->second;
        return true;
    }
    return false;
}
