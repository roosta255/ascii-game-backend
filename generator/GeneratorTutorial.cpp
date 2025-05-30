#include "GeneratorUtility.hpp"
#include "DoorEnum.hpp"
#include "Dungeon.hpp"
#include "GeneratorTutorial.hpp"
#include "iLayout.hpp"
#include "LayoutEnum.hpp"
#include "LayoutFlyweight.hpp"
#include "Match.hpp"

#include <drogon/drogon.h>

bool GeneratorTutorial::generate (int seed, Match& dst) const {
    constexpr auto LAYOUT = LAYOUT_2D_8x8;
    dst.dungeon.layout = LAYOUT;
    constexpr int w = DOOR_WALL;
    constexpr int o = DOOR_DOORWAY;
    bool success = true;

    LayoutFlyweight::getFlyweights().accessConst(LAYOUT, [&](const LayoutFlyweight& flyweight){
        flyweight.layout.accessConst([&](const iLayout& layoutIntf){
            GeneratorUtility util(dst.dungeon.rooms, layoutIntf);
            // if (!util.setupHorizontalWalls({w,w,w,w,w,w,w,w}, 1, 0)) {
            //     success = false;
            //     LOG_ERROR << "GeneratorTutorial::generate failed to setupHorizontalWalls";
            // }
            // if (!util.setupVerticalWalls({w,o,o,w,o,o,w}, 1, 0)) {
            //     success = false;
            //     LOG_ERROR << "GeneratorTutorial::generate failed to setupVerticalWalls";
            // }
            success &= util.setupDoorway(int4{0,0,0,0}, Cardinal::east());
            success &= util.setupTogglerBlue(int4{1,0,0,0}, Cardinal::north());
            success &= util.setupTogglerOrange(int4{1,1,0,0}, Cardinal::east());
            success &= util.setupTogglerBlue(int4{2,1,0,0}, Cardinal::east());
            success &= util.setupKeeper(int4{2,1,0,0}, Cardinal::south(), true);
            success &= util.setupKeeper(int4{2,0,0,0}, Cardinal::east(), true);

            success &= util.setupKeeper(int4{2,2,0,0}, Cardinal::west(), false);
            success &= util.setupKeeper(int4{2,2,0,0}, Cardinal::south(), false);
            success &= util.setupKeeper(int4{2,1,0,0}, Cardinal::west(), false);

            success &= util.setupShifter(int4{3,1,0,0}, Cardinal::north(), false);
            success &= util.setupShifter(int4{3,2,0,0}, Cardinal::west(), false);
        });
    });

    if (!success) {
        LOG_ERROR << "GeneratorTutorial::generate failed to setup something";
    }
    return success;
}