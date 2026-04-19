#pragma once
#include "iActivator.hpp"

class ActivatorDamageBite : public iActivator {
public:
    bool activate(ActivationContext& activation) const override;
};
