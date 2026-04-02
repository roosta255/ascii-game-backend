#pragma once

#include <nlohmann/json.hpp>
#include "DoorEnum.hpp"
#include "ItemEnum.hpp"
#include "RoleEnum.hpp"
#include "RuleFlyweight.hpp"
#include "TraitEnum.hpp"
#include "WrapperConfig.hpp"

struct RuleFlyweightApiView {
    static nlohmann::json buildAll() {
        nlohmann::json result = nlohmann::json::array();
        int i = 0;
        for (const auto& fw : RuleFlyweight::getFlyweights()) {
            result.push_back({
                {"index",      i++},
                {"name",       fw.name},
                {"matches",    serializeMatches(fw.config.matches)},
                {"conditions", serializeConditions(fw.config.conditions)},
                {"costs",      serializeCosts(fw.config.costs)}
            });
        }
        return result;
    }

private:
    static nlohmann::json traitsToJson(const TraitBits& bits) {
        nlohmann::json arr = nlohmann::json::array();
        for (int i = 0; i < TRAIT_COUNT; i++) {
            if (bits[i].orElse(false))
                arr.push_back(action_to_text(i));
        }
        return arr;
    }

    static nlohmann::json serializeCondition(const WrapperConfig::Condition& cond) {
        return {
            {"required",   traitsToJson(cond.required)},
            {"restricted", traitsToJson(cond.restricted)}
        };
    }

    static nlohmann::json serializeMatch(const WrapperConfig::Match& m) {
        nlohmann::json doorsArr = nlohmann::json::array();
        for (int i = 0; i < WrapperConfig::MAX_MATCH_LIST && m.doors[i] != DOOR_COUNT; i++)
            doorsArr.push_back((int)m.doors[i]);
        nlohmann::json rolesArr = nlohmann::json::array();
        for (int i = 0; i < WrapperConfig::MAX_MATCH_LIST && m.roles[i] != ROLE_COUNT; i++)
            rolesArr.push_back((int)m.roles[i]);
        nlohmann::json itemsArr = nlohmann::json::array();
        for (int i = 0; i < WrapperConfig::MAX_MATCH_LIST && m.items[i] != ITEM_NIL; i++)
            itemsArr.push_back(item_to_text(m.items[i]));
        return {
            {"traits", traitsToJson(m.traits)},
            {"doors",  doorsArr},
            {"roles",  rolesArr},
            {"items",  itemsArr}
        };
    }

    static nlohmann::json serializeMatches(const WrapperConfig::Matches& m) {
        return {
            {"actor",  serializeMatch(m.actor)},
            {"tool",   serializeMatch(m.tool)},
            {"target", serializeMatch(m.target)}
        };
    }

    static nlohmann::json serializeConditions(const WrapperConfig::Conditions& c) {
        return {
            {"actor",  serializeCondition(c.actor)},
            {"tool",   serializeCondition(c.tool)},
            {"target", serializeCondition(c.target)}
        };
    }

    static nlohmann::json serializeCosts(const WrapperConfig::Costs& costs) {
        nlohmann::json items = nlohmann::json::array();
        for (int i = 0; i < WrapperConfig::MAX_COSTS; i++) {
            if (costs.item[i] == ITEM_NIL) break;
            items.push_back(item_to_text(costs.item[i]));
        }
        return {
            {"items",  items},
            {"action", costs.action},
            {"move",   costs.move}
        };
    }
};
