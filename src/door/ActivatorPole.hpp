#pragma once

#include "iActivator.hpp"

class ActivatorPole : public iActivator {
public:
    bool activate(ActivationContext& activation) const override;
};
