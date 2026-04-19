#pragma once
#include "iActivator.hpp"

class ActivatorDamagePierce : public iActivator {
public:
    bool activate(ActivationContext& activation) const override;
};
