#pragma once

#include <functional>

namespace Json {
    class Value;
}

class RoleFlyweight;

struct Character {
    int damage;
    int role;
    int feats;
    int actions;
    int moves;

    bool accessRole(std::function<void(const RoleFlyweight&)>)const;

    bool isMovable()const;

    void toJson(Json::Value& out) const;
    bool fromJson(const Json::Value& in);
};