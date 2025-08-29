#pragma once

#include "iActivator.hpp"

class ActivatorKeeper : public iActivator {
public:
    CodeEnum activate(Activation& activation) const override;
}; 