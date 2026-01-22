#pragma once

#include "iActivator.hpp"

class ActivatorLightningRod : public iActivator {
public:
    CodeEnum activate(Activation& activation) const override;
};
