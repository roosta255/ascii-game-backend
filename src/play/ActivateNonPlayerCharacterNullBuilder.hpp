#pragma once

#include "iActivator.hpp"

class ActivateNonPlayerCharacterNullBuilder : public iActivator {
public:
    bool activate(Activation& activation) const override;
};