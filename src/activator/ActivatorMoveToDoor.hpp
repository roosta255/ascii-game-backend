#pragma once

#include "iActivator.hpp"
#include "CodeEnum.hpp"

class ActivatorMoveToDoor : public iActivator {
public:
    bool activate(Activation& activation) const override;
};
