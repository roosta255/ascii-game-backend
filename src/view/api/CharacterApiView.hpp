#pragma once

#include "adl_serializer.hpp"
#include "Character.hpp"
#include "Keyframe.hpp"
#include "KeyframeView.hpp"
#include "KeyframeFlyweight.hpp"
#include "RoleFlyweight.hpp"
#include <string>
#include <nlohmann/json.hpp>

struct CharacterApiView
{
    int damage = 0;
    std::string role = "UNPARSED_ROLE";
    int feats = 0;
    int actions = 0;
    int actionsRemaining = 0;
    int moves = 0;
    int movesRemaining = 0;
    bool isHidden = true;
    bool isObject = false;
    bool isActionable = false;
    int offset = -1;
    Array<KeyframeView, Character::MAX_KEYFRAMES> keyframes;

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

        this->keyframes = model.keyframes.transform([&](const Keyframe& keyframe){return KeyframeView(keyframe);});

        // offset
        params.match.containsCharacter(model, this->offset);

        // role
        RoleFlyweight::getFlyweights().accessConst(model.role, [&](const RoleFlyweight& flyweight) {
            this->role = flyweight.name;
            this->isObject = flyweight.isObject;
            this->isActionable = flyweight.isActionable;
            this->actionsRemaining = flyweight.actions - model.actions;
            this->movesRemaining = flyweight.moves - model.moves;
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
        {"actionsRemaining", view.actionsRemaining},
        {"moves", view.moves},
        {"movesRemaining", view.movesRemaining},
        {"isObject", view.isObject},
        {"isActionable", view.isActionable},
        {"keyframes", view.keyframes}
    };
}

inline void from_json(const nlohmann::json& j, CharacterApiView& view) {
    // TODO: log an error or leave empty
}
