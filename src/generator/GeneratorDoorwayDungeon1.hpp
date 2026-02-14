#pragma once

#include "Cardinal.hpp"
#include "iGenerator.hpp"
#include "int4.hpp"
#include "Rack.hpp"
#include "Room.hpp"
// provides entrance
// provides neighbors

class GeneratorDoorwayDungeon1 : public iGenerator {
public:
    constexpr static auto SKIP_SEED = 6666;
    constexpr static auto BOSS_ROOM_ID = 17;
    bool generate(int seed, Match&, Codeset& codeset) const override;
};
