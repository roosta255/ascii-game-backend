#pragma once

#include "iActivator.hpp"
#include "CodeEnum.hpp"

class ActivatorInactiveDoor : public iActivator {
public:
    CodeEnum activate(Activation& activation) const override;
}; 