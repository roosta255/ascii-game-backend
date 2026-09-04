#pragma once

#include "BehaviorEventEnum.hpp"
#include "iActivator.hpp"

// Queues a behavior trigger from the acting character to the current target
// character (target id is -1 when there is no target character), mirroring the
// controller.pushTrigger(nullptr, ...) calls activators previously made by hand.
class ActivatorPushBehaviorTrigger : public iActivator {
public:
    ActivatorPushBehaviorTrigger() = default;
    explicit ActivatorPushBehaviorTrigger(BehaviorEventEnum event) : event(event) {}

    BehaviorEventEnum event = BEHAVIOR_EVENT_NIL;

    bool activate(ActivationContext& activation) const override;
};
