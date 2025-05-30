#include "Titan.hpp"

#include <drogon/drogon.h>
#include <json/json.h>

void Titan::toJson(Json::Value& out) const {
    Json::Value playerJson;
    player.toJson(playerJson);
    out["player"] = playerJson;
}

bool Titan::fromJson(const Json::Value& in) {
    if (!player.fromJson(in["player"])) {
        LOG_ERROR << "Titan json couldnt parse player field";
        return false;
    }

    return true;
}
