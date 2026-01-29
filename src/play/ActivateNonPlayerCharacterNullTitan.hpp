#pragma once

#include "iActivator.hpp"

class ActivateNonPlayerCharacterNullTitan : public iActivator {
public:
    bool activate(Activation& activation) const override;
}; 