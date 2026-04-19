#pragma once

#include "iActivator.hpp"

class ActivatorLadder : public iActivator {
public:
    bool activate(ActivationContext& activation) const override;
};
