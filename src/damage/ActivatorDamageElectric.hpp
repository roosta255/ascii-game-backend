#pragma once
#include "iActivator.hpp"

class ActivatorDamageElectric : public iActivator {
public:
    bool activate(Activation& activation) const override;
};
