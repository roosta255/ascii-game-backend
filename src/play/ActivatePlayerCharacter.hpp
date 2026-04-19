#pragma once

#include "iActivator.hpp"

class ActivatePlayerCharacter : public iActivator {
public:
    bool activate(ActivationContext& activation) const override;
}; 