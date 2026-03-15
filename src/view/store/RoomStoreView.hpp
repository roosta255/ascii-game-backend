#pragma once

#include "adl_serializer.hpp"
#include "Array.hpp"
#include "Dungeon.hpp"
#include "Room.hpp"
#include "RoomFlyweight.hpp"
#include "Wall.hpp"
#include "WallStoreView.hpp"
#include <string>
#include <nlohmann/json.hpp>

struct RoomStoreView
{
    int visibility = ~0x0;
    Array<WallStoreView, 4> walls;
    std::string type = "UNPARSED_ROOM";
    int anterior = -1, posterior = -1, above = -1, below = -1;
    int roomId;

    inline RoomStoreView() = default;

    inline RoomStoreView(const Room& model)
    : walls(model.walls.convert<WallStoreView>())
    , visibility(model.visibility)
    , anterior(model.anterior)
    , posterior(model.posterior)
    , above(model.above)
    , below(model.below)
    , roomId(model.roomId)
    {
        RoomFlyweight::getFlyweights().accessConst(model.type, [&](const RoomFlyweight& flyweight) {
            this->type = flyweight.name;
        });
    }

    inline operator Room() const {
        Room model{
            .visibility = this->visibility,
            .walls = this->walls.convert<Wall>(),
            .anterior = this->anterior,
            .posterior = this->posterior,
            .above = this->above,
            .below = this->below,
            .roomId = this->roomId
        };
        RoomFlyweight::indexByString(this->type, model.type);
        return model;
    }
};

inline void to_json(nlohmann::json& j, const RoomStoreView& v) {
    j = {
        {"walls", v.walls}, {"type", v.type},
        {"anterior", v.anterior}, {"posterior", v.posterior},
        {"above", v.above}, {"below", v.below},
        {"roomId", v.roomId}, {"visibility", v.visibility}
    };
}

inline void from_json(const nlohmann::json& j, RoomStoreView& v) {
    j.at("walls").get_to(v.walls);
    j.at("type").get_to(v.type);
    j.at("anterior").get_to(v.anterior);
    j.at("posterior").get_to(v.posterior);
    j.at("above").get_to(v.above);
    j.at("below").get_to(v.below);
    j.at("roomId").get_to(v.roomId);
    v.visibility = j.value("visibility", ~0x0);
}
