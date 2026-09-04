#pragma once

#include "EventEnum.hpp"
#include "iActivator.hpp"

// Logs `event` with the acting character's role as actor and
// ActivationContext::resolvedItem as the item component — the same
// {ROLE, role} / {} / {ITEM, item} shape ActivatorGiveItem, ActivatorDeposit, and
// ActivatorPickpocket each wrote by hand. No-ops (returns false) if nothing was
// resolved this activation.
class ActivatorLogResolvedItemEvent : public iActivator {
public:
    ActivatorLogResolvedItemEvent() = default;
    explicit ActivatorLogResolvedItemEvent(EventEnum event) : event(event) {}

    EventEnum event = EVENT_NIL;

    bool activate(ActivationContext& activation) const override;
};
