#pragma once

#include "iActivator.hpp"

class ActivatorAttack : public iActivator {
public:
    bool activate(Activation& activation) const override;
};
