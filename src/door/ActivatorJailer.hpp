#pragma once

#include "iActivator.hpp"

class ActivatorJailer : public iActivator {
public:
    CodeEnum activate(Activation& activation) const override;
};
