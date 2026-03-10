#include "Inventory.hpp"
#include "Character.hpp"
#include "Dungeon.hpp"
#include "ItemFlyweight.hpp"
#include "Rack.hpp"
#include "RoleFlyweight.hpp"
#include "TraitFlyweight.hpp"
#include "TraitModifier.hpp"

TraitModifier::TraitModifier(
    std::initializer_list<TraitEnum> requiredList,
    std::initializer_list<TraitEnum> restrictedList,
    std::initializer_list<TraitEnum> setList,
    std::initializer_list<TraitEnum> clearList)
{
    applyList(required, requiredList);
    applyList(restricted, restrictedList);
    applyList(set, setList);
    applyList(clear, clearList);
}

TraitModifier::TraitModifier(std::initializer_list<std::initializer_list<TraitEnum>> nested) {
    auto it = nested.begin();
    
    // Safety check: ensure we have exactly 4 lists to prevent out-of-bounds
    if (nested.size() == 4) {
        applyList(required,   *it++);
        applyList(restricted, *it++);
        applyList(set,        *it++);
        applyList(clear,      *it++);
    }
}

void TraitModifier::applyList(
    TraitBits& bits,
    std::initializer_list<TraitEnum> list)
{
    for (auto t : list)
    {
        if (t == TRAIT_NIL) continue;
        bits.setIndexOn((size_t)t);
    }
}

Maybe<TraitModifier::TraitComputation> TraitModifier::computeCharacterTraits
(
    const Character& character
)
{
    TraitBits current; // starts empty

    // Flatten all modifiers into a single list
    constexpr auto MAX_STACK = (size_t)TRAIT_COUNT * 4; // safe cap
    CodeEnum error;
    character.accessRole(error, [&](const RoleFlyweight& flyweight){
        current = current | flyweight.traitsSourced;
    });
    current = current | character.traitsAfflicted;

    return computeTraits(current);
}

bool TraitModifier::applyAffliction(Character& character) const {
    bool applied = false;
    computeCharacterTraits(character).accessConst([&](const TraitComputation& computed) {
        if ((computed.final & required) != required) return;
        if ((computed.final & restricted).isAny()) return;
        character.traitsAfflicted = character.traitsAfflicted | set;
        character.traitsAfflicted = character.traitsAfflicted & ~clear;
        applied = true;
    });
    return applied;
}

Maybe<TraitModifier::TraitComputation> TraitModifier::computeTraits(const TraitBits& traits)
{
    TraitComputation result;
    result.initial = traits;
    
    TraitBits current = traits;
    constexpr auto MAX_ITERATIONS = (size_t)TRAIT_COUNT * 4;

    bool changed = true;
    size_t iterations = 0;

    while (changed && iterations++ < MAX_ITERATIONS)
    {
        changed = false;

        int i = 0;
        for (const auto& flyweight : TraitFlyweight::getFlyweights()) {
            const bool isTraitCurrentlyOn = current[i++].orElse(false);
            
            for (const auto& modifier : flyweight.modifiers) {
                if (!isTraitCurrentlyOn && !modifier.isGlobal) continue;
                if ((current & modifier.required) != modifier.required) continue;
                if ((current & modifier.restricted).isAny()) continue;

                TraitBits before = current;

                // Apply changes
                current = (current | modifier.set) & (~modifier.clear);

                if (before != current) {
                    changed = true;
                    
                    // Track bits that were turned ON (present in current, wasn't in before)
                    result.added = result.added | (current & ~before);
                    
                    // Track bits that were turned OFF (was in before, missing in current)
                    result.cleared = result.cleared | (before & ~current);
                }
            }
        }
    }

    if (iterations >= MAX_ITERATIONS) return Maybe<TraitComputation>::empty();

    result.final = current;
    return result;
}
