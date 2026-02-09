#pragma once

#include "iActivator.hpp"
#include "CodeEnum.hpp"

class ActivatorEndTurn : public iActivator {
public:
    bool activate(Activation& activation) const override;
};
