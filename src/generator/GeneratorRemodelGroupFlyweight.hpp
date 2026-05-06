#pragma once

#include "Array.hpp"
#include "GeneratorRemodelGroupEnum.hpp"
#include "GeneratorRemodelVariant.hpp"
#include <vector>

struct GeneratorRemodelGroupFlyweight {
    std::vector<GeneratorRemodelVariant> remodels;
    static const Array<GeneratorRemodelGroupFlyweight, REMODEL_GROUP_COUNT>& getFlyweights();
};
