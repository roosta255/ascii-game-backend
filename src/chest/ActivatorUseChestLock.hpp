#pragma once

#include "iActivator.hpp"

class ActivatorUseChestLock : public iActivator {
public:
    bool activate(ActivationContext& activation) const override;
};
