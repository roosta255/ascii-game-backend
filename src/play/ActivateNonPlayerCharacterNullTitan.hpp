#pragma once

#include "iActivator.hpp"

class ActivateNonPlayerCharacterNullTitan : public iActivator {
public:
    bool activate(ActivationContext& activation) const override;
}; 