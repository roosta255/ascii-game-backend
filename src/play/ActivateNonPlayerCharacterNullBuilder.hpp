#pragma once

#include "iActivator.hpp"

class ActivateNonPlayerCharacterNullBuilder : public iActivator {
public:
    CodeEnum activate(Activation& activation) const override;
};