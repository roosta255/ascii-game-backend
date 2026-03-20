#pragma once
#include "iActivator.hpp"

class ActivatorDamageFire : public iActivator {
public:
    bool activate(Activation& activation) const override;
};
