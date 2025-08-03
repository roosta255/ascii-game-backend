#pragma once

#include "iActivator.hpp"

class ActivatePlayerCharacter : public iActivator {
public:
    CodeEnum activate(Activation& activation) const override;
}; 