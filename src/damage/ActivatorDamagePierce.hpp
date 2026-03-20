#pragma once
#include "iActivator.hpp"

class ActivatorDamagePierce : public iActivator {
public:
    bool activate(Activation& activation) const override;
};
