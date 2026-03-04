#pragma once

#include <string>

#include "Array.hpp"
#include "LockEnum.hpp"
#include "Pointer.hpp"

class iActivator;

struct LockFlyweight {
    std::string name;
    bool isLocked = false;
    Pointer<const iActivator> activator;

    static const Array<LockFlyweight, LOCK_COUNT>& getFlyweights();
    static bool indexByString(const std::string& name, LockEnum& output);
};
