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
    //   actor  — required, roles  (actor is always a character)
    //   tool   — required, items  (tool is always an item)
    //   target — required, roles/items/doors (chosen by actual target type)
    // All active (non-empty) filters must pass. Lists use ANY semantics: the
    // slot's value need only appear once in the list.
    struct Match {
        TraitBits required = {};
        TraitBits restricted = {};
        DoorEnum  doors[MAX_MATCH_LIST] = { DOOR_COUNT, DOOR_COUNT, DOOR_COUNT, DOOR_COUNT,
                                            DOOR_COUNT, DOOR_COUNT, DOOR_COUNT, DOOR_COUNT };
        RoleEnum  roles[MAX_MATCH_LIST] = { ROLE_COUNT, ROLE_COUNT, ROLE_COUNT, ROLE_COUNT,
                                            ROLE_COUNT, ROLE_COUNT, ROLE_COUNT, ROLE_COUNT };
        ItemEnum  items[MAX_MATCH_LIST] = { ITEM_UNALLOCATED, ITEM_UNALLOCATED, ITEM_UNALLOCATED, ITEM_UNALLOCATED,
                                            ITEM_UNALLOCATED, ITEM_UNALLOCATED, ITEM_UNALLOCATED, ITEM_UNALLOCATED };
        // Chest lock match: all bits must be present in the target chest's lock's lockAttributes.
        TraitBits locks = {};

        bool isAny() const {
            return required.isAny()
                || restricted.isAny()
                || doors[0] != DOOR_COUNT
                || roles[0] != ROLE_COUNT
                || items[0] != ITEM_UNALLOCATED
                || locks.isAny();
        }

        // Returns true if required and roles filters pass for a character.
        // items/doors/locks fields are not evaluated here.
        bool matchesCharacter(const TraitBits& computed, RoleEnum role) const;

        // Returns true if required and items filters pass for an item.
        // roles/doors/locks fields are not evaluated here.
        bool matchesItem(const TraitBits& attributes, ItemEnum type) const;

        // Returns true if required and doors filters pass for a wall/door.
        // roles/items/locks fields are not evaluated here.
        bool matchesDoor(const TraitBits& attributes, DoorEnum door) const;
    };

    struct Matches {
        Match actor  = {};
        Match tool   = {};
        Match target = {};
    };

    struct Condition {
        TraitBits required = {};
        TraitBits restricted = {};

        // Returns true if the computed required satisfy required (all present) and restricted (none present).
        bool passesCharacter(const TraitBits& computed) const;
    };

    struct Conditions {
        Condition actor = {};
        Condition tool = {};
        Condition target = {};
    };

    struct Costs {
        ItemEnum item[MAX_COSTS] = { ITEM_UNALLOCATED, ITEM_UNALLOCATED };
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
            && costs.item[0] == ITEM_UNALLOCATED
            && costs.move == 0
            && costs.action == 0;
    }
};

inline bool WrapperConfig::Match::matchesCharacter(const TraitBits& computed, RoleEnum role) const {
    if (required.isAny() && (required - computed).isAny()) return false;
    if (restricted.isAny() && (restricted & computed).isAny()) return false;
    if (roles[0] != ROLE_COUNT) {
        bool found = false;
        for (int i = 0; i < WrapperConfig::MAX_MATCH_LIST && roles[i] != ROLE_COUNT; i++)
            if (role == roles[i]) { found = true; break; }
        if (!found) return false;
    }
    return true;
}

inline bool WrapperConfig::Match::matchesItem(const TraitBits& attributes, ItemEnum type) const {
    if (required.isAny() && (required - attributes).isAny()) return false;
    if (restricted.isAny() && (restricted & attributes).isAny()) return false;
    if (items[0] != ITEM_UNALLOCATED) {
        bool found = false;
        for (int i = 0; i < WrapperConfig::MAX_MATCH_LIST && items[i] != ITEM_UNALLOCATED; i++)
            if (type == items[i]) { found = true; break; }
        if (!found) return false;
    }
    return true;
}

inline bool WrapperConfig::Match::matchesDoor(const TraitBits& attributes, DoorEnum door) const {
    if (required.isAny() && (required - attributes).isAny()) return false;
    if (restricted.isAny() && (restricted & attributes).isAny()) return false;
    if (doors[0] != DOOR_COUNT) {
        bool found = false;
        for (int i = 0; i < WrapperConfig::MAX_MATCH_LIST && doors[i] != DOOR_COUNT; i++)
            if (door == doors[i]) { found = true; break; }
        if (!found) return false;
    }
    return true;
}

inline bool WrapperConfig::Condition::passesCharacter(const TraitBits& computed) const {
    return !(required - computed).isAny() && !(restricted & computed).isAny();
}
