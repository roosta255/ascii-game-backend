#pragma once

#include "iActivator.hpp"
#include "CodeEnum.hpp"

class ActivatorInactiveDoor : public iActivator {
public:
    bool activate(ActivationContext& activation) const override;
}; 