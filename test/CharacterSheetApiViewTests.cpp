#include <algorithm>
#include <catch2/catch_test_macros.hpp>

#include "Character.hpp"
#include "CharacterSheetApiView.hpp"
#include "RoleEnum.hpp"
#include "TraitBits.hpp"
#include "TraitEnum.hpp"
#include "TraitModifier.hpp"

// Helper: find a StreamedTraitView by trait name in a vector
static const CharacterSheetApiView::StreamedTraitView* findTrait(
    const std::vector<CharacterSheetApiView::StreamedTraitView>& vec,
    const std::string& name)
{
    auto it = std::find_if(vec.begin(), vec.end(),
        [&](const CharacterSheetApiView::StreamedTraitView& v){ return v.trait == name; });
    return it != vec.end() ? &(*it) : nullptr;
}

// Helper: build a TraitComputation with only .final set
static TraitModifier::TraitComputation makeComputed(std::initializer_list<TraitEnum> traits) {
    TraitModifier::TraitComputation computed;
    computed.final = makeTraitBits(traits);
    return computed;
}

// ---------------------------------------------------------------------------
// Default construction
// ---------------------------------------------------------------------------

TEST_CASE("CharacterSheetApiView: default constructor produces empty sheet", "[sheet]") {
    CharacterSheetApiView view;
    REQUIRE(view.afflictions.empty());
    REQUIRE(view.attributes.empty());
    REQUIRE(view.capabilities.empty());
    REQUIRE(view.characters.empty());
    REQUIRE(view.debuffs.empty());
    REQUIRE(view.states.empty());
    REQUIRE(view.roles.empty());
}

// ---------------------------------------------------------------------------
// Filtering: only present or cleared traits are listed
// ---------------------------------------------------------------------------

TEST_CASE("CharacterSheetApiView: absent non-cleared trait is not listed", "[sheet]") {
    Character character;
    character.role = ROLE_BUILDER;
    // Nothing in final, nothing in cleared
    const auto computed = makeComputed({});

    CharacterSheetApiView view(character, computed);

    // DEXTERITY is in the flyweights but not present or cleared — not listed
    REQUIRE(findTrait(view.attributes, "DEXTERITY") == nullptr);
    REQUIRE(findTrait(view.afflictions, "SNAKE_BITE") == nullptr);
}

TEST_CASE("CharacterSheetApiView: present trait is listed with isPresent true", "[sheet]") {
    Character character;
    character.role = ROLE_BUILDER;
    const auto computed = makeComputed({TRAIT_DEXTERITY});

    CharacterSheetApiView view(character, computed);

    const auto* entry = findTrait(view.attributes, "DEXTERITY");
    REQUIRE(entry != nullptr);
    REQUIRE(entry->isPresent == true);
}

// ---------------------------------------------------------------------------
// Color
// ---------------------------------------------------------------------------

TEST_CASE("CharacterSheetApiView: present trait with no upstream gets gray color", "[sheet]") {
    Character character;
    character.role = ROLE_BUILDER;
    // DEXTERITY with nothing in final that would set it via upstream
    const auto computed = makeComputed({TRAIT_DEXTERITY});

    CharacterSheetApiView view(character, computed);

    const auto* entry = findTrait(view.attributes, "DEXTERITY");
    REQUIRE(entry != nullptr);
    REQUIRE(entry->isPresent == true);
    REQUIRE(entry->upstream.empty());
}

TEST_CASE("CharacterSheetApiView: present trait with upstream does not get gray color", "[sheet]") {
    Character character;
    character.role = ROLE_BUILDER;
    // DEXTERITY_DEBUFF is set by SNAKE_BITE; with both in final,
    // DEXTERITY_DEBUFF should have upstream entries pointing to the affliction
    const auto computed = makeComputed({TRAIT_DEXTERITY_DEBUFF, TRAIT_SNAKE_BITE});

    CharacterSheetApiView view(character, computed);

    const auto* entry = findTrait(view.debuffs, "DEXTERITY_DEBUFF");
    REQUIRE(entry != nullptr);
    REQUIRE(entry->isPresent == true);
    REQUIRE(!entry->upstream.empty());
}

TEST_CASE("CharacterSheetApiView: cleared trait does not get gray color", "[sheet]") {
    Character character;
    character.role = ROLE_BUILDER;
    TraitModifier::TraitComputation computed;
    computed.cleared = makeTraitBits({TRAIT_DEXTERITY});

    CharacterSheetApiView view(character, computed);

    const auto* entry = findTrait(view.attributes, "DEXTERITY");
    REQUIRE(entry != nullptr);
    REQUIRE(entry->isPresent == false);
}

TEST_CASE("CharacterSheetApiView: cleared trait is listed with isPresent false", "[sheet]") {
    Character character;
    character.role = ROLE_BUILDER;
    // DEXTERITY was cleared during computation but is not in final
    TraitModifier::TraitComputation computed;
    computed.cleared = makeTraitBits({TRAIT_DEXTERITY});

    CharacterSheetApiView view(character, computed);

    const auto* entry = findTrait(view.attributes, "DEXTERITY");
    REQUIRE(entry != nullptr);
    REQUIRE(entry->isPresent == false);
}

TEST_CASE("CharacterSheetApiView: trait in both final and cleared shows as present", "[sheet]") {
    Character character;
    character.role = ROLE_BUILDER;
    TraitModifier::TraitComputation computed;
    computed.final   = makeTraitBits({TRAIT_DEXTERITY});
    computed.cleared = makeTraitBits({TRAIT_DEXTERITY});

    CharacterSheetApiView view(character, computed);

    const auto* entry = findTrait(view.attributes, "DEXTERITY");
    REQUIRE(entry != nullptr);
    REQUIRE(entry->isPresent == true); // final wins
}

// ---------------------------------------------------------------------------
// Affliction categorization
// ---------------------------------------------------------------------------

TEST_CASE("CharacterSheetApiView: present affliction appears in afflictions vector", "[sheet]") {
    Character character;
    character.role = ROLE_BUILDER;
    const auto computed = makeComputed({TRAIT_SNAKE_BITE});

    CharacterSheetApiView view(character, computed);

    REQUIRE(!view.afflictions.empty());
    const auto* entry = findTrait(view.afflictions, "SNAKE_BITE");
    REQUIRE(entry != nullptr);
    REQUIRE(entry->isPresent == true);
}

TEST_CASE("CharacterSheetApiView: cleared affliction appears with isPresent false", "[sheet]") {
    Character character;
    character.role = ROLE_BUILDER;
    TraitModifier::TraitComputation computed;
    computed.cleared = makeTraitBits({TRAIT_SNAKE_BITE});

    CharacterSheetApiView view(character, computed);

    const auto* entry = findTrait(view.afflictions, "SNAKE_BITE");
    REQUIRE(entry != nullptr);
    REQUIRE(entry->isPresent == false);
}

// ---------------------------------------------------------------------------
// Attribute categorization
// ---------------------------------------------------------------------------

TEST_CASE("CharacterSheetApiView: present attribute appears in attributes vector", "[sheet]") {
    Character character;
    character.role = ROLE_BUILDER;
    const auto computed = makeComputed({TRAIT_STRENGTH});

    CharacterSheetApiView view(character, computed);

    const auto* entry = findTrait(view.attributes, "STRENGTH");
    REQUIRE(entry != nullptr);
    REQUIRE(entry->isPresent == true);
}

// ---------------------------------------------------------------------------
// Capability categorization
// ---------------------------------------------------------------------------

TEST_CASE("CharacterSheetApiView: present capability appears in capabilities vector", "[sheet]") {
    Character character;
    character.role = ROLE_BUILDER;
    const auto computed = makeComputed({TRAIT_ACTOR});

    CharacterSheetApiView view(character, computed);

    const auto* entry = findTrait(view.capabilities, "ACTOR");
    REQUIRE(entry != nullptr);
    REQUIRE(entry->isPresent == true);
}

TEST_CASE("CharacterSheetApiView: absent non-cleared capability is not listed", "[sheet]") {
    Character character;
    character.role = ROLE_BUILDER;
    const auto computed = makeComputed({});

    CharacterSheetApiView view(character, computed);

    REQUIRE(findTrait(view.capabilities, "ACTOR") == nullptr);
}

// ---------------------------------------------------------------------------
// Debuff categorization
// ---------------------------------------------------------------------------

TEST_CASE("CharacterSheetApiView: present debuff appears in debuffs vector", "[sheet]") {
    Character character;
    character.role = ROLE_BUILDER;
    const auto computed = makeComputed({TRAIT_DEXTERITY_DEBUFF});

    CharacterSheetApiView view(character, computed);

    const auto* entry = findTrait(view.debuffs, "DEXTERITY_DEBUFF");
    REQUIRE(entry != nullptr);
    REQUIRE(entry->isPresent == true);
}

// ---------------------------------------------------------------------------
// State categorization
// ---------------------------------------------------------------------------

TEST_CASE("CharacterSheetApiView: present state appears in both states and debuffs vectors", "[sheet]") {
    Character character;
    character.role = ROLE_BUILDER;
    const auto computed = makeComputed({TRAIT_IS_DEAD});

    CharacterSheetApiView view(character, computed);

    // IS_DEAD is both a state and a debuff
    const auto* stateEntry = findTrait(view.states, "IS_DEAD");
    REQUIRE(stateEntry != nullptr);
    REQUIRE(stateEntry->isPresent == true);

    const auto* debuffEntry = findTrait(view.debuffs, "IS_DEAD");
    REQUIRE(debuffEntry != nullptr);
    REQUIRE(debuffEntry->isPresent == true);
}

// ---------------------------------------------------------------------------
// Multi-category traits
// ---------------------------------------------------------------------------

TEST_CASE("CharacterSheetApiView: CHARACTER_DEXTERITY appears in both attributes and characters", "[sheet]") {
    Character character;
    character.role = ROLE_BUILDER;
    const auto computed = makeComputed({TRAIT_DEXTERITY});

    CharacterSheetApiView view(character, computed);

    REQUIRE(findTrait(view.attributes, "DEXTERITY") != nullptr);
    REQUIRE(findTrait(view.characters, "DEXTERITY") != nullptr);
}

TEST_CASE("CharacterSheetApiView: CHARACTER_ACTOR appears in attributes, capabilities, and characters", "[sheet]") {
    Character character;
    character.role = ROLE_BUILDER;
    const auto computed = makeComputed({TRAIT_ACTOR});

    CharacterSheetApiView view(character, computed);

    REQUIRE(findTrait(view.attributes,   "ACTOR") != nullptr);
    REQUIRE(findTrait(view.capabilities, "ACTOR") != nullptr);
    REQUIRE(findTrait(view.characters,   "ACTOR") != nullptr);
}

// ---------------------------------------------------------------------------
// Role section: remaining and lost traits
// ---------------------------------------------------------------------------

TEST_CASE("CharacterSheetApiView: role traits are present when computed has all sourced traits", "[sheet]") {
    Character character;
    character.role = ROLE_BUILDER;
    // Builder sources: ACTOR, KEYER, FOGGY, ORGANIC
    const auto computed = makeComputed({
        TRAIT_ACTOR,
        TRAIT_KEYER,
        TRAIT_FOGGY,
        TRAIT_ORGANIC
    });

    CharacterSheetApiView view(character, computed);

    REQUIRE(!view.roles.empty());

    for (const auto& roleEntry : view.roles) {
        REQUIRE(roleEntry.isPresent == true);
    }

    const auto* actorEntry = findTrait(view.roles, "ACTOR");
    REQUIRE(actorEntry != nullptr);
    REQUIRE(actorEntry->isPresent == true);
}

TEST_CASE("CharacterSheetApiView: computed non-role traits show isPresent false in roles", "[sheet]") {
    Character character;
    character.role = ROLE_BUILDER;
    // IS_DEAD is not in BUILDER's traitsSourced -> traitsLost -> isPresent false
    const auto computed = makeComputed({TRAIT_IS_DEAD});

    CharacterSheetApiView view(character, computed);

    const auto* deadEntry = findTrait(view.roles, "IS_DEAD");
    REQUIRE(deadEntry != nullptr);
    REQUIRE(deadEntry->isPresent == false);
}

TEST_CASE("CharacterSheetApiView: role section mixes remaining and lost traits", "[sheet]") {
    Character character;
    character.role = ROLE_BUILDER;
    const auto computed = makeComputed({
        TRAIT_ACTOR,
        TRAIT_IS_DEAD
    });

    CharacterSheetApiView view(character, computed);

    const auto* actorEntry = findTrait(view.roles, "ACTOR");
    REQUIRE(actorEntry != nullptr);
    REQUIRE(actorEntry->isPresent == true);

    const auto* deadEntry = findTrait(view.roles, "IS_DEAD");
    REQUIRE(deadEntry != nullptr);
    REQUIRE(deadEntry->isPresent == false);
}

TEST_CASE("CharacterSheetApiView: ROLE_EMPTY has no role traits", "[sheet]") {
    Character character;
    character.role = ROLE_EMPTY;
    const auto computed = makeComputed({});

    CharacterSheetApiView view(character, computed);

    REQUIRE(view.roles.empty());
}

// ---------------------------------------------------------------------------
// Upstream/downstream vector cap
// ---------------------------------------------------------------------------

TEST_CASE("CharacterSheetApiView: upstream and downstream are capped at 2 entries", "[sheet]") {
    Character character;
    character.role = ROLE_BUILDER;
    const auto computed = makeComputed({TRAIT_SNAKE_BITE});

    CharacterSheetApiView view(character, computed);

    const auto* entry = findTrait(view.afflictions, "SNAKE_BITE");
    REQUIRE(entry != nullptr);
    REQUIRE(entry->upstream.size() <= 2);
    REQUIRE(entry->downstream.size() <= 2);
}
