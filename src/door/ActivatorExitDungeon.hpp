#pragma once

#include "iActivator.hpp"

class ActivatorExitDungeon : public iActivator {
public:
    bool activate(Activation& activation) const override;
};
