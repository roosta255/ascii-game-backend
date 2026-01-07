#pragma once

#include "iActivator.hpp"
#include "CodeEnum.hpp"

class ActivatorInactiveItem : public iActivator {
public:
    CodeEnum activate(Activation& activation) const override;
}; 