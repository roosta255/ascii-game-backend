#pragma once

#include "iActivator.hpp"

class ActivatorUseChestLock : public iActivator {
public:
    bool activate(Activation& activation) const override;
};
