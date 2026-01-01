#pragma once

#include "adl_serializer.hpp"
#include "Character.hpp"
#include "Keyframe.hpp"
#include "KeyframeView.hpp"
#include "KeyframeFlyweight.hpp"
#include "RoleFlyweight.hpp"
#include <string>
#include <nlohmann/json.hpp>

struct CharacterStoreView
{
    int damage = 0;
    std::string role = "UNPARSED_ROLE";
    int feats = 0;
    int actions = 0;
    int moves = 0;
    int keys = 0;  // Number of keys held by the character
    int visibility = 0;
    Array<KeyframeView, Character::MAX_KEYFRAMES> keyframes;

    inline CharacterStoreView() = default;
    inline CharacterStoreView(const Character& model)
    : damage(model.damage), feats(model.feats), actions(model.actions), moves(model.moves), keys(model.keys), visibility(model.visibility), keyframes(model.keyframes.transform([&](const Keyframe& keyframe){return KeyframeView(keyframe);}))
    {
        RoleFlyweight::getFlyweights().accessConst(model.role, [&](const RoleFlyweight& flyweight) {
            this->role = flyweight.name;
        });
    }
    inline operator Character() const {
        Character model{
            .damage = this->damage,
            .feats = this->feats,
            .actions = this->actions,
            .moves = this->moves,
            .keys = this->keys,
            .visibility = this->visibility,
            .keyframes = this->keyframes.convert<Keyframe>()
        };
        RoleFlyweight::indexByString(this->role, model.role);
        return model;
    }
};

// Reflection-based JSON serialization
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CharacterStoreView, damage, role, feats, actions, moves, keys, keyframes)
