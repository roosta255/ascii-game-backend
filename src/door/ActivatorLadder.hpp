#pragma once

#include "iActivator.hpp"

class ActivatorLadder : public iActivator {
public:
    bool activate(Activation& activation) const override;
};
