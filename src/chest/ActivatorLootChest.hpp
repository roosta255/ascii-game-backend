#pragma once

#include "iActivator.hpp"

class ActivatorLootChest : public iActivator {
public:
    bool activate(ActivationContext& activation) const override;
};
