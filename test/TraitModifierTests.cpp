#include <catch2/catch_test_macros.hpp>
#include "Character.hpp"
#include "TraitBits.hpp"
#include "TraitEnum.hpp"
#include "TraitModifier.hpp"

// Helper: returns true if the given trait bit is set in the result
static bool hasTrait(const Maybe<TraitModifier::TraitComputation>& result, TraitEnum trait) {
    return result.orElse(TraitModifier::TraitComputation{}).final[static_cast<int>(trait)].orElse(false);
}

// AFFLICTION_SNAKE_BITE modifier:
//   required   = { CHARACTER_DEXTERITY }
//   restricted = { NIL }              <- ignored as a no-op placeholder
//   set        = { DERIVED_DEXTERITY_DEBUFF }
//   clear      = { CHARACTER_DEXTERITY }

TEST_CASE("TraitModifier required: modifier does not fire when required trait is absent", "[traits]") {
    Character character;
    // Snake bite present but CHARACTER_DEXTERITY is NOT set
    character.traitsAfflicted = makeTraitBits({TRAIT_SNAKE_BITE});

    auto result = TraitModifier::computeCharacterTraits(character);

    REQUIRE(result.isPresent());
    REQUIRE(!hasTrait(result, TRAIT_DEXTERITY_DEBUFF));
}

TEST_CASE("TraitModifier set: modifier sets bits when required traits are met", "[traits]") {
    Character character;
    // Both snake bite and CHARACTER_DEXTERITY present — modifier fires and sets the debuff
    character.traitsAfflicted = makeTraitBits({TRAIT_SNAKE_BITE, TRAIT_DEXTERITY});

    auto result = TraitModifier::computeCharacterTraits(character);

    REQUIRE(result.isPresent());
    REQUIRE(hasTrait(result, TRAIT_DEXTERITY_DEBUFF));
}

TEST_CASE("TraitModifier clear: modifier clears bits when it fires", "[traits]") {
    Character character;
    // Snake bite fires and clears CHARACTER_DEXTERITY
    character.traitsAfflicted = makeTraitBits({TRAIT_SNAKE_BITE, TRAIT_DEXTERITY});

    auto result = TraitModifier::computeCharacterTraits(character);

    REQUIRE(result.isPresent());
    REQUIRE(!hasTrait(result, TRAIT_DEXTERITY));
}

// DERIVED_IS_DEAD modifier:
//   required   = { CHARACTER_ORGANIC }
//   restricted = { CHARACTER_UNDEAD }
//   set        = { NIL }              <- ignored as a no-op placeholder
//   clear      = { CHARACTER_ACTOR, CHARACTER_FLYING, CHARACTER_KEYER }

TEST_CASE("TraitModifier restricted: modifier does not fire when a restricted trait is present", "[traits]") {
    Character character;
    // Dead + organic + undead: UNDEAD blocks the modifier, so ACTOR must survive
    character.traitsAfflicted = makeTraitBits({
        TRAIT_IS_DEAD,
        TRAIT_ORGANIC,
        TRAIT_UNDEAD,
        TRAIT_ACTOR
    });

    auto result = TraitModifier::computeCharacterTraits(character);

    REQUIRE(result.isPresent());
    REQUIRE(hasTrait(result, TRAIT_ACTOR));
}
