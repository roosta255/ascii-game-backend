#include "int4.hpp"
#include "int4_to_json.hpp"

#include <drogon/drogon.h>
#include <json/json.h>

void int4_to_json (const int4& src, Json::Value& out) {
    out["x"] = src[0];
    out["y"] = src[1];
    out["z"] = src[2];
    out["t"] = src[3];
}

bool int4_from_json (const Json::Value& in, int4& dst) {
    // x
    if (!in.isMember("x")) {
        LOG_ERROR << "int4 json is missing x field";
        return false;
    }
    dst[0] = in["x"].asInt();

    // y
    if (!in.isMember("y")) {
        LOG_ERROR << "int4 json is missing y field";
        return false;
    }
    dst[1] = in["y"].asInt();

    // z
    if (!in.isMember("z")) {
        LOG_ERROR << "int4 json is missing z field";
        return false;
    }
    dst[2] = in["z"].asInt();

    // t
    if (!in.isMember("t")) {
        LOG_ERROR << "int4 json is missing t field";
        return false;
    }
    dst[3] = in["t"].asInt();

    return true;
}
