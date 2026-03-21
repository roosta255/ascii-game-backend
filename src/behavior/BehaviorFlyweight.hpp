#pragma once

#include <string>

#include "Array.hpp"
#include "BehaviorEnum.hpp"
#include "BehaviorEventEnum.hpp"
#include "Pointer.hpp"

class iActivator;

struct BehaviorFlyweight {
    const char* name;
    Pointer<const iActivator> onMove;
    Pointer<const iActivator> onAttack;
    Pointer<const iActivator> onDamage;
    Pointer<const iActivator> onLoot;
    Pointer<const iActivator> onDeath;

    // Returns the iActivator Pointer for the given event type. May be empty if none defined.
    Pointer<const iActivator> getActivatorForEvent(BehaviorEventEnum event) const;

    static const Array<BehaviorFlyweight, BEHAVIOR_COUNT>& getFlyweights();
    static bool indexByString(const std::string& name, BehaviorEnum& output);
};
