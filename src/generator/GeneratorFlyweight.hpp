#pragma once

#include <string>
#include <vector>

#include "Array.hpp"
#include "GeneratorEnum.hpp"
#include "GeneratorRemodelVariant.hpp"
#include "LayoutEnum.hpp"
#include "Pointer.hpp"

class Codeset;
class iGenerator;
class Match;

struct GeneratorFlyweight {
    const char* name;
    bool isTest = false;
    LayoutEnum layout = LAYOUT_COUNT;
    int bossRoomId = -1;
    std::vector<GeneratorRemodelVariant> remodels;

    Pointer<const iGenerator> generator;

    static const Array<GeneratorFlyweight, GENERATOR_COUNT>& getFlyweights();
    static bool indexByString(const std::string& name, int& output);

    bool buildMatch(int seed, Match& dst, Codeset& codeset) const;
};
