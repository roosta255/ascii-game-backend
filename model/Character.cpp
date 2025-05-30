#include "Character.hpp"

#include <drogon/drogon.h>
#include <json/json.h>

#include "RoleFlyweight.hpp"

bool Character::isMovable() const
{
    int result = false;
    int movesTaken = moves;
    accessRole([&](const RoleFlyweight& flyweight){
        result = flyweight.isMovable && (flyweight.moves - movesTaken) > 0;
    });
    return result;
}

bool Character::accessRole
(
    std::function<void(const RoleFlyweight&)> consumer
) const {
    return RoleFlyweight::getFlyweights().accessConst(role, [&](const RoleFlyweight& flyweight){
        consumer(flyweight);
    });
}

void Character::toJson
(
    Json::Value& out
) const {
    out["damage"] = damage;
    out["feats"] = feats;
    out["actions"] = actions;
    out["moves"] = moves;

    // role
    RoleFlyweight::getFlyweights().accessConst(role, [&](const RoleFlyweight& flyweight){
        out["role"] = flyweight.name;
    });
}

bool Character::fromJson(
    const Json::Value& in
) {
    damage = in["damage"].asInt();
    feats = in["feats"].asInt();
    actions = in["actions"].asInt();
    moves = in["moves"].asInt();

    // role
    if (!in.isMember("role")) {
        LOG_ERROR << "Character json is missing role field";
        return false;
    }
    auto rolename = in["role"].asString();
    if (!RoleFlyweight::indexByString(rolename, role)) {
        LOG_ERROR << "Character json couldnt parse role " << rolename;
        return false;
    }

    return true;
}
