#include "ActivatorInactiveItem.hpp"
#include "Match.hpp"

CodeEnum ActivatorInactiveItem::activate(Activation& activation) const {
    return CODE_INACTIVE_ITEM;
} 