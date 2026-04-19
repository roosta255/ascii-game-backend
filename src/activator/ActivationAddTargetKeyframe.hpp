#pragma once

#include "iActivator.hpp"
#include "AnimationConfig.hpp"

class ActivationAddTargetKeyframe : public iActivator {
public:
    explicit ActivationAddTargetKeyframe(AnimationConfig cfg) : cfg(cfg) {}
    AnimationConfig cfg;
    bool activate(ActivationContext& activation) const override;
};
