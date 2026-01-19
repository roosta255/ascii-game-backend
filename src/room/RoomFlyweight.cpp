#include "Array.hpp"
#include "Bitcard.hpp"
#include "RoomEnum.hpp"
#include "RoomFlyweight.hpp"
#include "Match.hpp"
#include "iActivator.hpp"
#include "ActivatorToggler.hpp"
#include <unordered_map>

const Array<RoomFlyweight, ROOM_COUNT>& RoomFlyweight::getFlyweights()
{
    static auto flyweights = [](){
        Array<RoomFlyweight, ROOM_COUNT> flyweights;

        #define ROOM_DECL( name_, width_, height_, isBlockingDoor_, isBlockingLock_ ) \
            flyweights.getPointer( ROOM_##name_ ).access([&](RoomFlyweight& flyweight){ \
                flyweight.name = #name_; \
                flyweight.width = width_; \
                flyweight.height = height_; \
                flyweight.isBlockingDoor = isBlockingDoor_; \
                flyweight.isBlockingLock = isBlockingLock_; \
            });
        #include "Room.enum"
        #undef ROOM_DECL

        return flyweights;
    }();
    return flyweights;
}

bool RoomFlyweight::indexByString
(
    const std::string& name,
    RoomEnum& output
) {
    static const auto nameMap = [](){
        std::unordered_map<std::string, RoomEnum> nameMap;
        const auto& flyweights = getFlyweights();
        int i = 0;
        for(const RoomFlyweight& flyweight: flyweights) {
            nameMap[flyweight.name] = RoomEnum(i++);
        }
        return nameMap;
    }();
    if (auto search = nameMap.find(name); search != nameMap.end()) {
        output = search->second;
        return true;
    }
    return false;
}
