#pragma once

#include "iActivator.hpp"

class ActivatorAttack : public iActivator {
public:
    bool activate(ActivationContext& activation) const override;
};
