#pragma once

#include "iActivator.hpp"
#include "CodeEnum.hpp"

class ActivatorToggler : public iActivator {
public:
    CodeEnum activate(Activation& activation) const override;
};
