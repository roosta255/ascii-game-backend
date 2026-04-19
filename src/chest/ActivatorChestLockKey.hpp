#pragma once

#include "iActivator.hpp"

class ActivatorChestLockKey : public iActivator {
public:
    bool activate(ActivationContext& activation) const override;
};
