#pragma once

#include "int4.hpp"

namespace Json {
    class Value;
}

void int4_to_json(const int4& src, Json::Value& out);
bool int4_from_json(const Json::Value& in, int4& dst);
