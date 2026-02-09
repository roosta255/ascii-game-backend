#pragma once

#include "iActivator.hpp"
#include "CodeEnum.hpp"

class ActivatorUseDoor : public iActivator {
public:
    bool activate(Activation& activation) const override;
};
