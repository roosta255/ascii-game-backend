#pragma once

#include "Cardinal.hpp"
#include "iGenerator.hpp"
#include "int4.hpp"
#include "Rack.hpp"
#include "Room.hpp"
// provides entrance
// provides neighbors

class GeneratorDoorwayDungeon2 : public iGenerator {
public:
    constexpr static auto ENTRANCE_ROOM_ID = 55;
    constexpr static auto BOSS_ROOM_ID = 4;
    bool generate(int seed, Match&, Codeset& codeset) const override;
};
