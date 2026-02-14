#pragma once

#include <string>

#include "Array.hpp"
#include "ActionEnum.hpp"
#include "ActionType.hpp"
#include "Pointer.hpp"

class iActivator;

struct ActionFlyweight {
    std::string name;
    Pointer<const iActivator> activator;
    ActionType type;

    static const Array<ActionFlyweight, ACTION_COUNT>& getFlyweights();
    static const iActivator& getMoveToDoor();
    static const iActivator& getMoveToFloor();
    static const iActivator& getUseCharacter();
    static const iActivator& getUseDoor();
    static const iActivator& getUseInventoryItem();
    static const iActivator& getUseLock();
    static bool indexByString(const std::string& name, ActionEnum& output);
};
