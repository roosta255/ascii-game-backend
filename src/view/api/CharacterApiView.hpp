#pragma once

#include "adl_serializer.hpp"
#include "Character.hpp"
#include "RoleFlyweight.hpp"
#include <string>
#include <nlohmann/json.hpp>

struct CharacterApiView
{
    int damage = 0;
    std::string role = "UNPARSED_ROLE";
    int feats = 0;
    int actions = 0;
    int moves = 0;
    int keys = 0;  // Number of keys held by the character
    bool isHidden = true;
    bool isObject = false;
    int offset = -1;

    inline CharacterApiView() = default;

    inline CharacterApiView(const Character& model, const MatchApiParameters& params)
    : isHidden(params.isHidden(model.visibility))
    {
        // hide masked characters
        if (isHidden) return;

        this->damage = model.damage;
        this->feats = model.feats;
        this->actions = model.actions;
        this->moves = model.moves;
        this->keys = model.keys;

        // offset
        params.match.containsCharacter(model, this->offset);

        // role
        RoleFlyweight::getFlyweights().accessConst(model.role, [&](const RoleFlyweight& flyweight) {
            this->role = flyweight.name;
            this->isObject = flyweight.isObject;
        });
    }

};

inline void to_json(nlohmann::json& j, const CharacterApiView& view) {
    // default or debugging version, if you still want one
    if (view.isHidden) {
        // hidden
        j = {
            {"offset", view.offset},
            {"isHidden", true}
        };
        return;
    }
    // shown
    j = {
        {"offset", view.offset},
        {"damage", view.damage},
        {"role", view.role},
        {"feats", view.feats},
        {"actions", view.actions},
        {"moves", view.moves},
        {"isObject", view.isObject},
        {"keys", view.keys}
    };
}

inline void from_json(const nlohmann::json& j, CharacterApiView& view) {
    // TODO: log an error or leave empty
}
