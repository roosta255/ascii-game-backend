#pragma once

#include <string>

#include "Array.hpp"
#include "RemodelEnum.hpp"
#include "Pointer.hpp"

class iRemodel;

struct RemodelFlyweight {
    const char* name;
    Pointer<const iRemodel> remodel;
    bool isBlocker = false;

    static const Array<RemodelFlyweight, REMODEL_COUNT>& getFlyweights();
    static bool indexByString(const std::string& name, RemodelEnum& output);
};
