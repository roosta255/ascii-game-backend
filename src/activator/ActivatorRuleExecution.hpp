#pragma once

#include "iActivator.hpp"

class ActivatorRuleExecution : public iActivator {
public:
    bool activate(ActivationContext& activation) const override;
};
