#pragma once
#include "iActivator.hpp"

// Takes the first transferable pickpocketable item from the actor's own inventory
// and places it into the target chest (targetCharacterId). Requires TRAIT_HANDS.
// Pushes BEHAVIOR_EVENT_DEPOSIT on success.
class ActivatorDeposit : public iActivator {
public:
    bool activate(ActivationContext& activation) const override;
};
