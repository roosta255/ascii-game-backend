#pragma once
#include "iActivator.hpp"

// Steals an eligible item (FOOD or COIN) from the target character and deposits
// it into an unlocked chest in the room. Requires the subject to have
// TRAIT_PICKPOCKETER + TRAIT_HANDS, and the target to have TRAIT_PICKPOCKETABLE
// without TRAIT_ACTION_READY. Any character role may use this action.
class ActivatorPickpocket : public iActivator {
public:
    bool activate(ActivationContext& activation) const override;
};
