#pragma once

#include "iActivator.hpp"
#include "CodeEnum.hpp"

class ActivatorToggler : public iActivator {
public:
    bool activate(Activation& activation) const override;
};
