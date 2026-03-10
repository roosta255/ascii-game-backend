#pragma once

#include <initializer_list>
#include "Inventory.hpp"
#include "Maybe.hpp"
#include "Pointer.hpp"
#include "Rack.hpp"
#include "TraitBits.hpp"

class Character;
class Dungeon;

struct TraitModifier
{
    struct TraitComputation {
        TraitBits initial;
        TraitBits final;
        TraitBits added;
        TraitBits cleared;
    };

    bool isGlobal = false; // global traits will evaluate even if the current traitbits is off. please use copious required/restricted lists to correctly evaluate
    TraitBits required;
    TraitBits restricted;
    TraitBits set;
    TraitBits clear;

    TraitModifier() = default;

    TraitModifier(
        std::initializer_list<TraitEnum> requiredList,
        std::initializer_list<TraitEnum> restrictedList,
        std::initializer_list<TraitEnum> setList,
        std::initializer_list<TraitEnum> clearList);

    TraitModifier(std::initializer_list<std::initializer_list<TraitEnum>> nested);

    static Maybe<TraitComputation> computeCharacterTraits(const Character& character);

    // Validates required/restricted against character's computed traits, then applies
    // set/clear directly to character.traitsAfflicted. Caller must update traitsComputed.
    bool applyAffliction(Character& character) const;

private:

    static Maybe<TraitComputation> computeTraits(const TraitBits& traits);

    static void applyList(
        TraitBits& bits,
        std::initializer_list<TraitEnum> list);
};
