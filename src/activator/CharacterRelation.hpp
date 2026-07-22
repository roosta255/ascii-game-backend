#pragma once

// Selects which existing ActivationContext character an activator starts from.
enum class CharacterAnchor {
    Actor,
    Target
};

// Selects which character, relative to the anchor, an activator should operate on.
//
// Self returns the anchor character unchanged. Primary/Secondary/Tertiary resolve
// to the complementary character sharing every one of the anchor's role traits
// except the relationship trait (TRAIT_PRIMARY/TRAIT_SECONDARY/TRAIT_TERTIARY) —
// see ActivationContext::resolveCharacter().
enum class CharacterRelation {
    Self,
    Primary,
    Secondary,
    Tertiary
};
