#include "Codeset.hpp"
#include "DoorEnum.hpp"
#include "Dungeon.hpp"
#include "DungeonAuthor.hpp"
#include "GeneratorTutorial.hpp"
#include "iLayout.hpp"
#include "LayoutEnum.hpp"
#include "LayoutFlyweight.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "RoleEnum.hpp"

bool GeneratorTutorial::generate (int seed, Match& dst, Codeset& codeset) const {
    MatchController controller(dst, codeset);
    constexpr auto LAYOUT = LAYOUT_2D_8x8;
    dst.dungeon.layout = LAYOUT;
    bool success = true;

    CodeEnum error = CODE_UNKNOWN_ERROR;
    if (!dst.setupSingleBuilder(error))
        return false;

    // TODO: starting locations
    int floorId = 0, builderId, roomId = 0;
    dst.builders.access(0, [&](Builder& builder){
        builder.character.role = ROLE_BUILDER;
        if (dst.containsCharacter(builder.character, builderId)) {
            bool isBuilderFloorFree = controller.findFreeFloor(roomId, CHANNEL_CORPOREAL, floorId);
            if (isBuilderFloorFree) {
                controller.assignCharacterToFloor(builderId, roomId, CHANNEL_CORPOREAL, floorId);
            }
        }
    });

    LayoutFlyweight::getFlyweights().accessConst(LAYOUT, [&](const LayoutFlyweight& flyweight){
        flyweight.layout.accessConst([&](const iLayout& layoutIntf){
            layoutIntf.setupAdjacencyPointers(dst.dungeon.rooms);
            DungeonAuthor util(controller, layoutIntf);

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

            const auto createToggler = [&](const int4& coord){
                int outCharacterId = -1, outFloorId = -1;
                codeset.addFailure(!(success &= util.setupTogglerSwitch(coord, outCharacterId, outFloorId)));
                codeset.addFailure(!(success &= outCharacterId != -1), CODE_NEW_CHARACTER_BUT_BAD_ID);
            };
            createToggler(int4{1,0,0,0});
            createToggler(int4{1,1,0,0});
            createToggler(int4{2,2,0,0});
            createToggler(int4{3,0,0,0});
        });
    });

    if (!success) {
        return false;
    }

    return true;
}