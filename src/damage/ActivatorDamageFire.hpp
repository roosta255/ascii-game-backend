#pragma once
#include "iActivator.hpp"

class ActivatorDamageFire : public iActivator {
public:
    bool activate(ActivationContext& activation) const override;
};
