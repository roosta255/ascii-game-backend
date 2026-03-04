#pragma once

#include <string>

#include "Array.hpp"
#include "TraitEnum.hpp"
#include "TraitModifier.hpp"
#include "TraitType.hpp"

class iActivator;

struct TraitFlyweight {
    std::string name;
    TraitType type;
    static constexpr auto MAX_MODIFIERS = 8;
    Array<TraitModifier, MAX_MODIFIERS> modifierAllocation;
    Rack<TraitModifier> modifiers = Rack<TraitModifier>::buildEmptyStackFromArray<MAX_MODIFIERS>(modifierAllocation);;

    static const Array<TraitFlyweight, TRAIT_COUNT>& getFlyweights();
    static bool indexByString(const std::string& name, TraitEnum& output);
};
