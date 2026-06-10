#pragma once
#include "iActivator.hpp"

// Behavior activator for BEHAVIOR_EVENT_ACT.
// If the actor has TRAIT_PICKPOCKETER and a valid TRAIT_ACTION_READY action budget,
// pickpockets the first eligible victim in the room.
// Otherwise, if the actor has TRAIT_WANDERER and a remaining move budget, wanders
// to a free floor cell in the current room.
class ActivatorBehaviorActPickpocketWander : public iActivator {
public:
    bool activate(ActivationContext& activation) const override;
};
