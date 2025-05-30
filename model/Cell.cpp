#include "Cell.hpp"

#include <json/json.h>

void Cell::toJson(Json::Value& out) const {
    out["offset"] = offset;
}

bool Cell::fromJson(const Json::Value& in) {
    offset = in["offset"].asInt();
    return true;
}
