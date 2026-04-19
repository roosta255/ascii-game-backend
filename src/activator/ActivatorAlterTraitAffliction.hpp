#pragma once

#include "iActivator.hpp"
#include "TraitBits.hpp"

struct AlterTraitAfflictionSpec {
    TraitBits set;
    TraitBits clear;
};

struct AlterTraitAfflictionConfig {
    AlterTraitAfflictionSpec actor;
    AlterTraitAfflictionSpec target;
};

class ActivatorAlterTraitAffliction : public iActivator {
public:
    ActivatorAlterTraitAffliction() = default;
    explicit ActivatorAlterTraitAffliction(AlterTraitAfflictionConfig config) : config(config) {}

    AlterTraitAfflictionConfig config;
    bool activate(ActivationContext& activation) const override;
};
