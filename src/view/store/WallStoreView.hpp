#pragma once

#include "adl_serializer.hpp"
#include "KeyframeView.hpp"
#include "Wall.hpp"
#include "DoorFlyweight.hpp"
#include <string>
#include <nlohmann/json.hpp>

struct WallStoreView
{
    std::string door = "UNPARSED_DOOR";
    int adjacent = -1;
    Array<KeyframeView, Wall::MAX_KEYFRAMES> keyframes;
 
    inline WallStoreView() = default;
 
    inline WallStoreView(const Wall& model)
    : adjacent(model.adjacent)
    , keyframes(model.keyframes.convert<KeyframeView>())
    {
        DoorFlyweight::getFlyweights().accessConst(model.door, [&](const DoorFlyweight& flyweight) {
            this->door = flyweight.name;
        });
    }
 
    inline operator Wall() const
    {
        Wall model{
            .adjacent = this->adjacent
        };
        DoorFlyweight::indexByString(this->door, model.door);
        return model;
    }
};

inline void to_json(nlohmann::json& j, const WallStoreView& v) {
    j = {
        {"door", v.door}, {"adjacent", v.adjacent}, {"keyframes", v.keyframes}
    };
}

inline void from_json(const nlohmann::json& j, WallStoreView& v) {
    j.at("door").get_to(v.door);
    j.at("adjacent").get_to(v.adjacent);
    if (j.contains("keyframes")) j.at("keyframes").get_to(v.keyframes);
}
