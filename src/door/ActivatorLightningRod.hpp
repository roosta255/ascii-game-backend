#pragma once

#include "Cardinal.hpp"
#include "iActivator.hpp"

class ActivatorLightningRod : public iActivator {
public:
    static constexpr auto  LIGHTNING_ROD_WALL = Cardinal::north();
    bool activate(Activation& activation) const override;
};
