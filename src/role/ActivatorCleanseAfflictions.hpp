#pragma once

#include "iActivator.hpp"

class ActivatorCleanseAfflictions : public iActivator {
public:
    bool activate(ActivationContext& activation) const override;
};
