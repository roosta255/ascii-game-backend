#pragma once

#include "CharacterRelation.hpp"
#include "Maybe.hpp"
#include "iActivator.hpp"
#include "TraitBits.hpp"

struct AlterTraitAfflictionSpec {
    TraitBits set;
    TraitBits clear;
};

// Applies a spec to whichever character ActivationContext::resolveCharacter()
// finds for (anchor, relation) — e.g. Anchor: Actor, Relation: Secondary reaches
// the acting character's paired Processor/Receptacle-style character without a
// dedicated "secondary" activator class.
struct AlterTraitAfflictionRelatedSpec {
    CharacterAnchor anchor        = CharacterAnchor::Actor;
    CharacterRelation relation    = CharacterRelation::Secondary;
    AlterTraitAfflictionSpec spec;
};

struct AlterTraitAfflictionConfig {
    AlterTraitAfflictionSpec actor;
    AlterTraitAfflictionSpec target;
    Maybe<AlterTraitAfflictionRelatedSpec> related;
};

class ActivatorAlterTraitAffliction : public iActivator {
public:
    ActivatorAlterTraitAffliction() = default;
    explicit ActivatorAlterTraitAffliction(AlterTraitAfflictionConfig config) : config(config) {}

    AlterTraitAfflictionConfig config;
    bool activate(ActivationContext& activation) const override;
};
