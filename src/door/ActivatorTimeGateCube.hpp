#pragma once

#include "iActivator.hpp"

class ActivatorTimeGateCube : public iActivator {
public:
    bool activate(Activation& activation) const override;
};
