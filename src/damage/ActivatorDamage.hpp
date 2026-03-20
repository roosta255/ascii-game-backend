#pragma once

#include "iActivator.hpp"

class ActivatorDamage : public iActivator {
public:
    bool activate(Activation& activation) const override;
};
