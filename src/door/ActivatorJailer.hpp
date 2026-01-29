#pragma once

#include "iActivator.hpp"

class ActivatorJailer : public iActivator {
public:
    bool activate(Activation& activation) const override;
};
