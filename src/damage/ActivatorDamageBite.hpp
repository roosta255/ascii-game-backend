#pragma once
#include "iActivator.hpp"

class ActivatorDamageBite : public iActivator {
public:
    bool activate(Activation& activation) const override;
};
