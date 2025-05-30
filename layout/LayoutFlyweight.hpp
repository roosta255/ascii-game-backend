#pragma once

#include <string>

#include "Array.hpp"
#include "LayoutEnum.hpp"
#include "Pointer.hpp"

class iLayout;

struct LayoutFlyweight {
    const char* name;

    Pointer<const iLayout> layout;

    static const Array<LayoutFlyweight, LAYOUT_COUNT>& getFlyweights();
    static bool indexByString(const std::string& name, int& output);
};
