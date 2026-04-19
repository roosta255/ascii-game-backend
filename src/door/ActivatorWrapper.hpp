#pragma once

#include "iActivator.hpp"
#include "WrapperConfig.hpp"

class ActivatorWrapper : public iActivator {
public:
    ActivatorWrapper() = default;
    void init(const WrapperConfig& config);
    bool activate(ActivationContext& activation) const override;
private:
    WrapperConfig _config;
};
