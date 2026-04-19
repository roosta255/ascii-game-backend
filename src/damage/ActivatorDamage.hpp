#pragma once

#include "iActivator.hpp"

class ActivatorDamage : public iActivator {
public:
    bool activate(ActivationContext& activation) const override;
};
