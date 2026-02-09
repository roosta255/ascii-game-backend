#pragma once

#include "adl_serializer.hpp"
#include "Character.hpp"
#include "Keyframe.hpp"
#include "KeyframeView.hpp"
#include "KeyframeFlyweight.hpp"
#include "LocationView.hpp"
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
    int visibility = 0;
    Array<KeyframeView, Character::MAX_KEYFRAMES> keyframes;
    LocationView location;
    int characterId = -1;

    inline CharacterStoreView() = default;
    inline CharacterStoreView(const Character& model)
    : damage(model.damage), feats(model.feats), actions(model.actions), moves(model.moves), visibility(model.visibility), keyframes(model.keyframes.transform([&](const Keyframe& keyframe){return KeyframeView(keyframe);}))
    , location(model.location), characterId(model.characterId)
    {
        RoleFlyweight::getFlyweights().accessConst(model.role, [&](const RoleFlyweight& flyweight) {
            this->role = flyweight.name;
        });
    }
    inline operator Character() const {
        Character model{
            .keyframes = this->keyframes.convert<Keyframe>(),
            .location = this->location,
            .damage = this->damage,
            .feats = this->feats,
            .actions = this->actions,
            .moves = this->moves,
            .visibility = this->visibility,
            .characterId = this->characterId
        };
        RoleFlyweight::indexByString(this->role, model.role);
        return model;
    }
};

// Reflection-based JSON serialization
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CharacterStoreView, damage, role, feats, actions, moves, keyframes, location, characterId)
