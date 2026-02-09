#pragma once

#include "iActivator.hpp"
#include "CodeEnum.hpp"

class ActivatorUseLock : public iActivator {
public:
    bool activate(Activation& activation) const override;
};
