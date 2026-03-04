#pragma once

#include "iActivator.hpp"
#include "CodeEnum.hpp"

class ActivatorCritterBite : public iActivator {
public:
    bool activate(Activation& activation) const override;
};
