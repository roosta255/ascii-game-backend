#pragma once

#include "iActivator.hpp"

class ActivatorJailer : public iActivator {
public:
    bool activate(ActivationContext& activation) const override;
};
