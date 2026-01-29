#pragma once

#include "iActivator.hpp"

class ActivatorShifter : public iActivator {
public:
    bool activate(Activation& activation) const override;
}; 