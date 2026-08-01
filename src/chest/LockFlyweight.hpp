#pragma once

#include <string>

#include "Array.hpp"
#include "LockEnum.hpp"
#include "Pointer.hpp"
#include "TraitBits.hpp"

class iActivator;

struct LockFlyweight {
    std::string name;
    bool isLocked = false;
    Pointer<const iActivator> activator;
    bool isLockActionable = false;
    TraitBits lockAttributes = {};

    static const Array<LockFlyweight, LOCK_COUNT>& getFlyweights();
    static bool indexByString(const std::string& name, LockEnum& output);
    static bool findByTraits(const TraitBits& traits, LockEnum& output);
};
