#pragma once

#include "iActivator.hpp"

class ActivatorCleanseAfflictions : public iActivator {
public:
    bool activate(Activation& activation) const override;
};
