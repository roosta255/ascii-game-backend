#pragma once
#include "iActivator.hpp"

class ActivatorDamageCold : public iActivator {
public:
    bool activate(Activation& activation) const override;
};
