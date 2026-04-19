#pragma once

#include "iActivator.hpp"

class ActivatorValidateNotOccupied : public iActivator {
public:
    bool activate(ActivationContext& activation) const override;
};
