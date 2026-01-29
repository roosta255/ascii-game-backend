#pragma once

#include "iActivator.hpp"

class ActivatePlayerCharacter : public iActivator {
public:
    bool activate(Activation& activation) const override;
}; 