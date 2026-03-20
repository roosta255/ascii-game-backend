#pragma once

#include <string>

#include "Array.hpp"
#include "DamageTypeBits.hpp"
#include "Pointer.hpp"
#include "TraitBits.hpp"
#include "TraitModifier.hpp"
#include "ItemEnum.hpp"

class iActivator;

struct ItemFlyweight {
    std::string name;
    int stacks = 1;
    bool isActionable = false;
    TraitBits itemAttributes;
    DamageTypeBits damageTypes;
    static constexpr auto MAX_CARRY_MODIFIERS = 8;
    Array<TraitModifier, MAX_CARRY_MODIFIERS> carryModifierAllocation;
    Rack<TraitModifier> carryModifiers = Rack<TraitModifier>::buildEmptyStackFromArray<MAX_CARRY_MODIFIERS>(carryModifierAllocation);

    Pointer<const iActivator> useActivator;

    static const Array<ItemFlyweight, ITEM_COUNT>& getFlyweights();
    static bool indexByString(const std::string& name, ItemEnum& output);
};
