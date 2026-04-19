#pragma once
#include "iActivator.hpp"

class ActivatorDamageElectric : public iActivator {
public:
    bool activate(ActivationContext& activation) const override;
};
