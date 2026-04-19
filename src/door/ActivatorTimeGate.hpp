#pragma once

#include "iActivator.hpp"

class ActivatorTimeGate : public iActivator {
public:
    static constexpr auto TIME_GATE_DIRECTION = Cardinal::north();
    bool activate(ActivationContext& activation) const override;
};
