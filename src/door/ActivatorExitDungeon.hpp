#pragma once

#include "iActivator.hpp"

class ActivatorExitDungeon : public iActivator {
public:
    bool activate(ActivationContext& activation) const override;
};
