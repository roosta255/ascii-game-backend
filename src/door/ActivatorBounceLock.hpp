#pragma once

#include "iActivator.hpp"

class ActivatorBounceLock : public iActivator {
public:
    bool activate(Activation& activation) const override;
};
