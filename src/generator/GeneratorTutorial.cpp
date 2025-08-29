#include "GeneratorUtility.hpp"
#include "DoorEnum.hpp"
#include "Dungeon.hpp"
#include "GeneratorTutorial.hpp"
#include "iLayout.hpp"
#include "LayoutEnum.hpp"
#include "LayoutFlyweight.hpp"
#include "Match.hpp"
#include "RoleEnum.hpp"

bool GeneratorTutorial::generate (int seed, Match& dst) const {
    constexpr auto LAYOUT = LAYOUT_2D_8x8;
    dst.dungeon.layout = LAYOUT;
    constexpr int w = DOOR_WALL;
    constexpr int o = DOOR_DOORWAY;
    bool success = true;

    CodeEnum error = CODE_UNKNOWN_ERROR;
    if (!dst.setupSingleBuilder(error))
        return false;

    for(auto& builder: dst.builders) {
        builder.character.role = ROLE_BUILDER;
    }

    LayoutFlyweight::getFlyweights().accessConst(LAYOUT, [&](const LayoutFlyweight& flyweight){
        flyweight.layout.accessConst([&](const iLayout& layoutIntf){
            GeneratorUtility util(dst.dungeon.rooms, layoutIntf);
            success &= util.setupDoorway(int4{0,0,0,0}, Cardinal::east());
            success &= util.setupDoorway(int4{0,2,0,0}, Cardinal::north());
            success &= util.setupTogglerBlue(int4{1,0,0,0}, Cardinal::north());
            success &= util.setupTogglerOrange(int4{1,1,0,0}, Cardinal::east());
            success &= util.setupTogglerBlue(int4{2,1,0,0}, Cardinal::east());

            success &= util.setupKeeper(int4{2,1,0,0}, Cardinal::south(), true);
            success &= util.setupKeeper(int4{2,0,0,0}, Cardinal::east(), true);

            success &= util.setupKeeper(int4{2,2,0,0}, Cardinal::west(), false);
            success &= util.setupKeeper(int4{2,2,0,0}, Cardinal::south(), false);
            success &= util.setupKeeper(int4{1,2,0,0}, Cardinal::west(), false);
            success &= util.setupKeeper(int4{3,1,0,0}, Cardinal::north(), false);

            success &= util.setupShifter(int4{3,2,0,0}, Cardinal::west(), false);

            Room& entrance = layoutIntf.getEntrance(dst.dungeon.rooms);
            entrance.floorCells.access(0, [&](Cell& cell){
                dst.builders.access(0, [&](Builder& builder){
                    if (!dst.containsCharacter(builder.character, cell.offset)) {
                        return;
                    }
                    success = true;
                });
            });
        });
    });

    Character toggler;
    toggler.role = ROLE_TOGGLER;
    toggler.visibility = ~0x0;
    success &= dst.addCharacterToFloor(toggler, 1); // 7448
    success &= dst.addCharacterToFloor(toggler, 3); // 7472
    success &= dst.addCharacterToFloor(toggler, 9); // 7496
    success &= dst.addCharacterToFloor(toggler, 18); // 7520

    if (!success) {
        return false;
    }

    return true;
}