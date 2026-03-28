#pragma once

#include "iActivator.hpp"

class ActivatorValidateNotOccupied : public iActivator {
public:
    bool activate(Activation& activation) const override;
};
