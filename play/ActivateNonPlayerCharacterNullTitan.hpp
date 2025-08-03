#pragma once

#include "iActivator.hpp"

class ActivateNonPlayerCharacterNullTitan : public iActivator {
public:
    CodeEnum activate(Activation& activation) const override;
}; 