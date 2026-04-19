#pragma once

#include "iActivator.hpp"

class ActivatorKeeper : public iActivator {
public:
    bool activate(ActivationContext& activation) const override;
}; 