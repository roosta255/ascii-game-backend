#pragma once
#include "iActivator.hpp"

class ActivatorDamageCrush : public iActivator {
public:
    bool activate(ActivationContext& activation) const override;
};
