#pragma once

#include "ActivatorEffect.hpp"
#include "DoorEnum.hpp"
#include "ItemEnum.hpp"
#include "RoleEnum.hpp"
#include "TraitBits.hpp"

struct WrapperConfig {
    static constexpr int MAX_EFFECTS = 8;
    static constexpr int MAX_COSTS = 2;
    static constexpr int MAX_MATCH_LIST = 8;

    // Per-slot match config. Checks that apply:
    //   actor  — traits, roles  (actor is always a character)
    //   tool   — traits, items  (tool is always an item)
    //   target — traits, roles/items/doors (chosen by actual target type)
    // All active (non-empty) filters must pass. Lists use ANY semantics: the
    // slot's value need only appear once in the list.
    struct Match {
        TraitBits traits = {};
        DoorEnum  doors[MAX_MATCH_LIST] = { DOOR_COUNT, DOOR_COUNT, DOOR_COUNT, DOOR_COUNT,
                                            DOOR_COUNT, DOOR_COUNT, DOOR_COUNT, DOOR_COUNT };
        RoleEnum  roles[MAX_MATCH_LIST] = { ROLE_COUNT, ROLE_COUNT, ROLE_COUNT, ROLE_COUNT,
                                            ROLE_COUNT, ROLE_COUNT, ROLE_COUNT, ROLE_COUNT };
        ItemEnum  items[MAX_MATCH_LIST] = { ITEM_NIL, ITEM_NIL, ITEM_NIL, ITEM_NIL,
                                            ITEM_NIL, ITEM_NIL, ITEM_NIL, ITEM_NIL };

        bool isAny() const {
            return traits.isAny()
                || doors[0] != DOOR_COUNT
                || roles[0] != ROLE_COUNT
                || items[0] != ITEM_NIL;
        }
    };

    struct Matches {
        Match actor  = {};
        Match tool   = {};
        Match target = {};
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
