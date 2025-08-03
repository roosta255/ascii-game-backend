#include "int4.hpp"
#include "int4_to_json.hpp"
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
        return false;
    }
    dst[0] = in["x"].asInt();

    // y
    if (!in.isMember("y")) {
        return false;
    }
    dst[1] = in["y"].asInt();

    // z
    if (!in.isMember("z")) {
        return false;
    }
    dst[2] = in["z"].asInt();

    // t
    if (!in.isMember("t")) {
        return false;
    }
    dst[3] = in["t"].asInt();

    return true;
}
