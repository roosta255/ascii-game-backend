#pragma once

#include <string>

#include "Array.hpp"
#include "Bitcard.hpp"
#include "Pointer.hpp"
#include "RoomEnum.hpp"

struct RoomFlyweight {
    std::string name;
    int width, height;
    Bitcard isBlockingDoor, isBlockingLock;

    static const Array<RoomFlyweight, ROOM_COUNT>& getFlyweights();
    static bool indexByString(const std::string& name, RoomEnum& output);
};
