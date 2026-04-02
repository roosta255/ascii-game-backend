#pragma once

#include "iGenerator.hpp"

class GeneratorDoorwayDungeon3Traits : public iGenerator {
public:
    constexpr static auto BOSS_ROOM_ID = 61;
    constexpr static auto ENTRANCE_ROOM_ID = 55;
    bool generate(int seed, Match&, Codeset& codeset) const override;
};
