#pragma once

#include <string>

#include "Array.hpp"
#include "DoorEnum.hpp"
#include "Pointer.hpp"

class iActivator;

struct DoorFlyweight {
    bool blocking = true;
    bool isDoorway = false;
    const char* name;
    Pointer<const iActivator> activator;

    static const Array<DoorFlyweight, DOOR_COUNT>& getFlyweights();
    static bool indexByString(const std::string& name, int& output);
};
