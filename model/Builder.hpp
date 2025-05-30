#pragma once

#include <string>

#include "Character.hpp"
#include "Player.hpp"

namespace Json {
    class Value;
}

struct Builder {
    Player player;
    Character character;

    void toJson(Json::Value& out) const;
    bool fromJson(const Json::Value& in);
};