#pragma once

#include <string>

#include "Array.hpp"
#include "ConductEnum.hpp"

struct ConductFlyweight {
    const char* name;

    static const Array<ConductFlyweight, CONDUCT_COUNT>& getFlyweights();
    static bool indexByString(const std::string& name, ConductEnum& output);
};
