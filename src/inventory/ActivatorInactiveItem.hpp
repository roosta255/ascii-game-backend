#pragma once

#include "iActivator.hpp"
#include "CodeEnum.hpp"

class ActivatorInactiveItem : public iActivator {
public:
    bool activate(ActivationContext& activation) const override;
}; 