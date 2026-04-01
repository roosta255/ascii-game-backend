#pragma once

#include "ActivatorEffect.hpp"
#include "ItemEnum.hpp"
#include "TraitBits.hpp"

struct WrapperConfig {
    static constexpr int MAX_EFFECTS = 8;
    static constexpr int MAX_COSTS = 2;

    struct Matches {
        TraitBits actor = {};
        TraitBits tool = {};
        TraitBits target = {};
    };

    struct Condition {
        TraitBits required = {};
        TraitBits restricted = {};
    };

    struct Conditions {
        Condition actor = {};
        Condition tool = {};
        Condition target = {};
    };

    struct Costs {
        ItemEnum item[MAX_COSTS] = { ITEM_NIL, ITEM_NIL };
        int action = 0;
        int move = 0;
    };

    struct Effects {
        ActivatorEffect onSuccess[MAX_EFFECTS] = {};
        ActivatorEffect onFail[MAX_EFFECTS] = {};
    };

    int priority = 0;
    Matches matches = {};
    Conditions conditions = {};
    Costs costs = {};
    Effects effects = {};

    bool isEmpty() const {
        return std::holds_alternative<NoEffect>(effects.onSuccess[0])
            && std::holds_alternative<NoEffect>(effects.onFail[0])
            && !matches.actor.isAny()
            && !matches.tool.isAny()
            && !matches.target.isAny()
            && !conditions.actor.required.isAny()
            && !conditions.actor.restricted.isAny()
            && !conditions.tool.required.isAny()
            && !conditions.tool.restricted.isAny()
            && !conditions.target.required.isAny()
            && !conditions.target.restricted.isAny()
            && costs.item[0] == ITEM_NIL
            && costs.move == 0
            && costs.action == 0;
    }
};
