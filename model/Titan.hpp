#pragma once

#include <string>

#include "Player.hpp"

namespace Json {
    class Value;
}

struct Titan {
    Player player;

    void toJson(Json::Value& out) const;
    bool fromJson(const Json::Value& in);
};