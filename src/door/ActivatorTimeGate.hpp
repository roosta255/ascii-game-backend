#pragma once

#include "iActivator.hpp"

class ActivatorTimeGate : public iActivator {
public:
    static constexpr auto TIME_GATE_DIRECTION = Cardinal::north();
    CodeEnum activate(Activation& activation) const override;
};
