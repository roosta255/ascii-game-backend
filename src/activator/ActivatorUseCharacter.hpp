#pragma once

#include "iActivator.hpp"
#include "CodeEnum.hpp"

class ActivatorUseCharacter : public iActivator {
public:
    bool activate(ActivationContext& activation) const override;
};
