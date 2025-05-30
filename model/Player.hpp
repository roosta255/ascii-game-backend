#pragma once

#include <string>

namespace Json {
    class Value;
}

struct Player {
    std::string account;

    void toJson(Json::Value& out) const;
    bool fromJson(const Json::Value& in);
};