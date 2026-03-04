#pragma once

#include "iActivator.hpp"

class ActivatorLootChest : public iActivator {
public:
    bool activate(Activation& activation) const override;
};
