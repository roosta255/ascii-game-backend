#pragma once

#include "TriggerConfig.hpp"
#include "iActivator.hpp"

// TriggerWrapper is a value-type iActivator that holds a TriggerConfig and
// executes it when activate() is called.  One instance lives inside each
// BEHAVIOR_ON_*_DECL slot of a BehaviorFlyweight entry; the event type and
// required FSM state are implicit from which slot and which BEHAVIOR_DECL owns it.
//
// activate() evaluates matches against the actor's ConductMemory, then applies
// effects (memory mutations and/or a behavior state transition).
class TriggerWrapper : public iActivator {
public:
    TriggerWrapper() = default;
    explicit TriggerWrapper(const TriggerConfig& config) : _config(config) {}

    bool activate(ActivationContext& activation) const override;

private:
    TriggerConfig _config;
};
