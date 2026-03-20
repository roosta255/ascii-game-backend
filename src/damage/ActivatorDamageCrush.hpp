#pragma once
#include "iActivator.hpp"

class ActivatorDamageCrush : public iActivator {
public:
    bool activate(Activation& activation) const override;
};
