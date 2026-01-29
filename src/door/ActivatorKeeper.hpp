#pragma once

#include "iActivator.hpp"

class ActivatorKeeper : public iActivator {
public:
    bool activate(Activation& activation) const override;
}; 