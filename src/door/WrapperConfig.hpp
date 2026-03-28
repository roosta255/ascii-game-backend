#pragma once

#include "ActivatorEffect.hpp"
#include "ItemEnum.hpp"
#include "TraitBits.hpp"

struct WrapperConfig {
    static constexpr int MAX_EFFECTS = 8;
    static constexpr int MAX_COSTS = 2;

    ActivatorEffect effects[MAX_EFFECTS] = {};   // NoEffect{} sentinel terminates the list
    TraitBits required = {};
    TraitBits restricted = {};
    ItemEnum itemCost[MAX_COSTS] = { ITEM_NIL, ITEM_NIL };
    int moveCost = 0;
    int actionCost = 0;

    bool isEmpty() const {
        return std::holds_alternative<NoEffect>(effects[0])
            && !required.isAny()
            && !restricted.isAny()
            && itemCost[0] == ITEM_NIL
            && moveCost == 0
            && actionCost == 0;
    }
};
