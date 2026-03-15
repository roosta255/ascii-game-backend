#pragma once

#include "adl_serializer.hpp"
#include "Character.hpp"
#include "Keyframe.hpp"
#include "KeyframeView.hpp"
#include "AnimationFlyweight.hpp"
#include "LocationView.hpp"
#include "RoleFlyweight.hpp"
#include "TraitBits.hpp"
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
    TraitBits traitsAfflicted;

    inline CharacterStoreView() = default;
    inline CharacterStoreView(const Character& model)
    : damage(model.damage), feats(model.feats), actions(model.actions), moves(model.moves), visibility(model.visibility), keyframes(model.keyframes.transform([&](const Keyframe& keyframe){return KeyframeView(keyframe);}))
    , location(model.location), characterId(model.characterId), traitsAfflicted(model.traitsAfflicted)
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
            .characterId = this->characterId,
            .traitsAfflicted = this->traitsAfflicted,
        };
        RoleFlyweight::indexByString(this->role, model.role);
        return model;
    }
};

inline void to_json(nlohmann::json& j, const CharacterStoreView& v) {
    j = {
        {"damage", v.damage}, {"role", v.role}, {"feats", v.feats},
        {"actions", v.actions}, {"moves", v.moves}, {"visibility", v.visibility},
        {"keyframes", v.keyframes}, {"location", v.location},
        {"characterId", v.characterId}, {"traitsAfflicted", v.traitsAfflicted}
    };
}

inline void from_json(const nlohmann::json& j, CharacterStoreView& v) {
    j.at("damage").get_to(v.damage);
    j.at("role").get_to(v.role);
    j.at("feats").get_to(v.feats);
    j.at("actions").get_to(v.actions);
    j.at("moves").get_to(v.moves);
    if (j.contains("visibility")) j.at("visibility").get_to(v.visibility);
    j.at("keyframes").get_to(v.keyframes);
    j.at("location").get_to(v.location);
    j.at("characterId").get_to(v.characterId);
    // traitsAfflicted is new — default to empty TraitBits{} for old saves
    if (j.contains("traitsAfflicted")) j.at("traitsAfflicted").get_to(v.traitsAfflicted);
}
