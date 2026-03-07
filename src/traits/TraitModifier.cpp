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

Maybe<TraitBits> TraitModifier::computeCharacterTraits
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
    computeCharacterTraits(character).accessConst([&](const TraitBits& computed) {
        if ((computed & required) != required) return;
        if ((computed & restricted).isAny()) return;
        character.traitsAfflicted = character.traitsAfflicted | set;
        character.traitsAfflicted = character.traitsAfflicted & ~clear;
        applied = true;
    });
    return applied;
}

Maybe<TraitBits> TraitModifier::computeTraits(const TraitBits& traits)
{
    TraitBits current = traits;
    // Flatten all modifiers into a single list
    constexpr auto MAX_ITERATIONS = (size_t)TRAIT_COUNT * 4; // safe cap

    bool changed = true;
    size_t iterations = 0;

    while (changed && iterations++ < MAX_ITERATIONS)
    {
        changed = false;

        int i = 0;
        for (const auto& flyweight: TraitFlyweight::getFlyweights()) {
            const bool isTraitCurrentlyOn = current[i++].orElse(false);
            for (const auto& modifier: flyweight.modifiers) {
                // process modifier
                if (!isTraitCurrentlyOn && !modifier.isGlobal) {
                    continue;
                }
                if ((current & modifier.required) != modifier.required) {
                    continue;
                }
                if ((current & modifier.restricted).isAny()) {
                    continue;
                }

                TraitBits before = current;

                current = current | (modifier.set);
                current = current & (~modifier.clear);

                if (before != current)
                    changed = true;
            }
        }
    }

    return iterations >= MAX_ITERATIONS ? Maybe<TraitBits>::empty() : current;
}
