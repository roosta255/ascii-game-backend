#pragma once

#include "iActivator.hpp"

class ActivatorPole : public iActivator {
public:
    bool activate(Activation& activation) const override;
};
