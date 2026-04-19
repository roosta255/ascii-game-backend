#pragma once

#include "iActivator.hpp"

class ActivatorTimeGateCube : public iActivator {
public:
    bool activate(ActivationContext& activation) const override;
};
