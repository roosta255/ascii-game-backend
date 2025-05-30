#include "Builder.hpp"

#include <drogon/drogon.h>
#include <json/json.h>

void Builder::toJson(Json::Value& out) const {
    // character
    Json::Value characterJson;
    character.toJson(characterJson);
    out["character"] = characterJson;

    // player
    Json::Value playerJson;
    player.toJson(playerJson);
    out["player"] = playerJson;
}

bool Builder::fromJson(const Json::Value& in) {
    // character
    if (!character.fromJson(in["character"])) {
        LOG_ERROR << "Builder json couldnt parse character field";
        return false;
    }

    // player
    if (!player.fromJson(in["player"])) {
        LOG_ERROR << "Builder json couldnt parse player field";
        return false;
    }

    return true;
}
