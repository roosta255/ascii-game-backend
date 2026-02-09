#pragma once

#include "iActivator.hpp"
#include "CodeEnum.hpp"

class ActivatorMoveToFloor : public iActivator {
public:
    bool activate(Activation& activation) const override;
};
