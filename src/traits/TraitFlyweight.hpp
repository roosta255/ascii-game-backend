#pragma once

#include <string>

#include "Array.hpp"
#include "Bitstick.hpp"
#include "TraitBits.hpp"
#include "TraitEnum.hpp"
#include "TraitModifier.hpp"
#include "TraitType.hpp"

class iActivator;

struct TraitFlyweight {
    TraitEnum index;
    std::string name;
    Bitstick<TRAIT_TYPE_COUNT> types;
    static constexpr auto MAX_MODIFIERS = 8;
    Array<TraitModifier, MAX_MODIFIERS> modifierAllocation;
    Rack<TraitModifier> modifiers = Rack<TraitModifier>::buildEmptyStackFromArray<MAX_MODIFIERS>(modifierAllocation);;

    TraitBits upstreamSetters, upstreamClearers;
    TraitBits downstreamSetters, downstreamClearers;

    static const Array<TraitFlyweight, TRAIT_COUNT>& getFlyweights();
    static bool indexByString(const std::string& name, TraitEnum& output);

    TraitBits getUpstream(const TraitBits& computed) const;
    TraitBits getDownstream(const TraitBits& computed) const;
};
