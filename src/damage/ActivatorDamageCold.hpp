#pragma once
#include "iActivator.hpp"

class ActivatorDamageCold : public iActivator {
public:
    bool activate(ActivationContext& activation) const override;
};
