#pragma once

#include <string>

#include "Array.hpp"
#include "DoorEnum.hpp"

struct DoorFlyweight {
    bool blocking = true;
    const char* name;

    static const Array<DoorFlyweight, DOOR_COUNT>& getFlyweights();
    static bool indexByString(const std::string& name, int& output);
};
