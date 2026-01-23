#include "GeneratorUtility.hpp"
#include "DoorEnum.hpp"
#include "Dungeon.hpp"
#include "GeneratorTest.hpp"
#include "iLayout.hpp"
#include "LayoutEnum.hpp"
#include "LayoutFlyweight.hpp"
#include "Match.hpp"
#include "RoleEnum.hpp"

bool GeneratorTest::generate (int seed, Match& dst, Codeset& codeset) const {
    constexpr auto LAYOUT = LAYOUT_2D_8x8;
    dst.dungeon.layout = LAYOUT;
    bool success = true;

    CodeEnum error = CODE_UNKNOWN_ERROR;
    if (!dst.setupSingleBuilder(error))
        return false;

    for(auto& builder: dst.builders) {
        builder.character.role = ROLE_BUILDER;
    }

    LayoutFlyweight::getFlyweights().accessConst(LAYOUT, [&](const LayoutFlyweight& flyweight){
        flyweight.layout.accessConst([&](const iLayout& layoutIntf){
            GeneratorUtility util(dst.dungeon.rooms, layoutIntf, codeset);

            success &= util.setup2x5Room(int4{1,1,0,0});
            success &= util.setup2x5Room(int4{1,2,0,0});

            success &= util.setupDoorway(int4{0,0,0,0}, Cardinal::east());
            success &= util.setupDoorway(int4{0,1,0,0}, Cardinal::east());
            success &= util.setupDoorway(int4{0,2,0,0}, Cardinal::east());
            success &= util.setupDoorway(int4{0,3,0,0}, Cardinal::east());
            success &= util.setupDoorway(int4{0,4,0,0}, Cardinal::east());
            success &= util.setupDoorway(int4{0,5,0,0}, Cardinal::east());
            /*success &= util.setupDoorway(int4{0,1,0,0}, Cardinal::west());
            success &= util.setupDoorway(int4{0,2,0,0}, Cardinal::west());
            success &= util.setupDoorway(int4{0,3,0,0}, Cardinal::west());
            success &= util.setupDoorway(int4{0,4,0,0}, Cardinal::west());
            success &= util.setupDoorway(int4{0,5,0,0}, Cardinal::west());*/
            success &= util.setupDoorway(int4{1,1,0,0}, Cardinal::south());
            success &= util.setupDoorway(int4{1,2,0,0}, Cardinal::south());
            success &= util.setupDoorway(int4{1,3,0,0}, Cardinal::south());
            success &= util.setupDoorway(int4{1,4,0,0}, Cardinal::south());
            success &= util.setupDoorway(int4{1,5,0,0}, Cardinal::south());
            success &= util.setupDoorway(int4{2,1,0,0}, Cardinal::north());
            success &= util.setupDoorway(int4{3,1,0,0}, Cardinal::north());
            success &= util.setupDoorway(int4{4,1,0,0}, Cardinal::north());
            success &= util.setupDoorway(int4{5,1,0,0}, Cardinal::north());
            success &= util.setupDoorway(int4{6,1,0,0}, Cardinal::north());
            success &= util.setupDoorway(int4{2,1,0,0}, Cardinal::south());
            success &= util.setupDoorway(int4{3,1,0,0}, Cardinal::south());
            success &= util.setupDoorway(int4{4,1,0,0}, Cardinal::south());
            success &= util.setupDoorway(int4{5,1,0,0}, Cardinal::south());
            success &= util.setupDoorway(int4{6,1,0,0}, Cardinal::south());
            success &= util.setupDoorway(int4{2,1,0,0}, Cardinal::west());
            success &= util.setupDoorway(int4{3,1,0,0}, Cardinal::west());
            success &= util.setupDoorway(int4{4,1,0,0}, Cardinal::west());
            success &= util.setupDoorway(int4{5,1,0,0}, Cardinal::west());
            success &= util.setupDoorway(int4{6,1,0,0}, Cardinal::west());
            success &= util.setupDoorway(int4{6,1,0,0}, Cardinal::east());
            success &= util.setupDoorway(int4{6,0,0,0}, Cardinal::east());

            success &= util.setupKeeper(int4{1,1,0,0}, Cardinal::west(), true);
            success &= util.setupJailer(int4{1,2,0,0}, Cardinal::west(), false);

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

    if (!success) {
        return false;
    }

    return true;
}