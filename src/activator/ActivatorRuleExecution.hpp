#pragma once

#include "iActivator.hpp"

class ActivatorRuleExecution : public iActivator {
public:
    bool activate(Activation& activation) const override;
};
