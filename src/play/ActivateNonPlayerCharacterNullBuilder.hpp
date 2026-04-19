#pragma once

#include "iActivator.hpp"

class ActivateNonPlayerCharacterNullBuilder : public iActivator {
public:
    bool activate(ActivationContext& activation) const override;
};