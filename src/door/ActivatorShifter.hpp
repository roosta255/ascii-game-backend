#pragma once

#include "iActivator.hpp"

class ActivatorShifter : public iActivator {
public:
    bool activate(ActivationContext& activation) const override;
}; 