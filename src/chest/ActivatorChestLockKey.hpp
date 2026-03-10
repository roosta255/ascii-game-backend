#pragma once

#include "iActivator.hpp"

class ActivatorChestLockKey : public iActivator {
public:
    bool activate(Activation& activation) const override;
};
