#pragma once

#include "ItemEnum.hpp"
#include "iActivator.hpp"

class ActivatorGiveItem : public iActivator {
public:
    ActivatorGiveItem() = default;
    explicit ActivatorGiveItem(ItemEnum item) : item(item) {}

    ItemEnum item = ITEM_NIL;

    bool activate(ActivationContext& activation) const override;
};
