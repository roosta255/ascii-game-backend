#pragma once
#include "iActivator.hpp"

// Dispatches BEHAVIOR_EVENT_ACT to the acting character's behavior flyweight.
// Fired by NullTitan once per action/move budget unit for each NPC character
// with a non-NIL behavior.
class ActivatorNpcAct : public iActivator {
public:
    bool activate(ActivationContext& activation) const override;
};
