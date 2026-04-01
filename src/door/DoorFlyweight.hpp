#pragma once

#include <string>

#include "Array.hpp"
#include "DoorEnum.hpp"
#include "Pointer.hpp"
#include "TraitBits.hpp"

class iActivator;

struct DoorFlyweight {
    const char* name;
    TraitBits doorAttributes;
    bool isDoorActionable = false;
    bool isLockActionable = false;
    Pointer<const iActivator> lockActivator;
    Pointer<const iActivator> doorActivator;

    static const Array<DoorFlyweight, DOOR_COUNT>& getFlyweights();
    static bool indexByString(const std::string& name, DoorEnum& output);

    // Returns the first door whose doorAttributes exactly match the given bits.
    // Returns false if no door matches.
    static bool findByTraits(const TraitBits& traits, DoorEnum& output);
};
