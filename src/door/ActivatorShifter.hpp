#pragma once

#include "iActivator.hpp"

class ActivatorShifter : public iActivator {
public:
    CodeEnum activate(Activation& activation) const override;
}; 