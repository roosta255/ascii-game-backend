#pragma once

#include "iActivator.hpp"

class ActivatorTimeGateCube : public iActivator {
public:
    CodeEnum activate(Activation& activation) const override;
};
