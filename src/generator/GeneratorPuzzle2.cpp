#include "GeneratorUtility.hpp"
#include "DoorEnum.hpp"
#include "Dungeon.hpp"
#include "GeneratorPuzzle2.hpp"
#include "iLayout.hpp"
#include "LayoutEnum.hpp"
#include "LayoutFlyweight.hpp"
#include "Match.hpp"
#include "RoleEnum.hpp"

bool GeneratorPuzzle2::generate (int seed, Match& dst) const {
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
            success &= util.setupTogglerBlue(int4{0,0,0,0}, Cardinal::north());
            success &= util.setupTogglerOrange(int4{0,0,0,0}, Cardinal::east());

            Room& entrance = layoutIntf.getEntrance(dst.dungeon.rooms);
            entrance.getUsedFloorCells().access(0, [&](Cell& cell){
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
    success &= dst.addCharacterToFloor(toggler, 0); // 7448

    if (!success) {
        return false;
    }

    return true;
}