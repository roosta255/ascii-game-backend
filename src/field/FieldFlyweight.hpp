#pragma once
#include <cstdint>
#include <string>
#include "Array.hpp"
#include "FieldEnum.hpp"
#include "Pointer.hpp"

class iFieldResolver;
class iFieldSourceProvider;

struct FieldFlyweight {
    std::string name;
    Pointer<const iFieldResolver> resolver;
    Pointer<const iFieldSourceProvider> sourceProvider;
    int invalidationMask = 0;
    int16_t minValue = 0, maxValue = 0;

    static const Array<FieldFlyweight, FIELD_COUNT>& getFlyweights();
};
