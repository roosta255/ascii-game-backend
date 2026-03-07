#pragma once

#include <string>

#include "Array.hpp"
#include "GeneratorEnum.hpp"
#include "Pointer.hpp"

class iGenerator;

struct GeneratorFlyweight {
    const char* name;
    bool isTest = false;

    Pointer<const iGenerator> generator;

    static const Array<GeneratorFlyweight, GENERATOR_COUNT>& getFlyweights();
    static bool indexByString(const std::string& name, int& output);
};
