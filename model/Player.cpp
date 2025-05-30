#include "Player.hpp"
#include <drogon/drogon.h>
#include <json/json.h>

void Player::toJson(Json::Value& out) const {
    out["account"] = account;
}

bool Player::fromJson(const Json::Value& in) {
    if (!in.isMember("account")) {
        LOG_ERROR << "Player json is missing account field";
        return false;
    }

    account = in["account"].asString();

    return true;
}
