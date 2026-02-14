#include "Codeset.hpp"
#include "DoorEnum.hpp"
#include "Dungeon.hpp"
#include "DungeonAuthor.hpp"
#include "GeneratorPuzzle2.hpp"
#include "iLayout.hpp"
#include "LayoutEnum.hpp"
#include "LayoutFlyweight.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "RoleEnum.hpp"

bool GeneratorPuzzle2::generate (int seed, Match& dst, Codeset& codeset) const {
    MatchController controller(dst, codeset);
    constexpr auto LAYOUT = LAYOUT_2D_8x8;
    dst.dungeon.layout = LAYOUT;
    constexpr int w = DOOR_WALL;
    constexpr int o = DOOR_DOORWAY;
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

            success &= util.setupTogglerBlue(int4{0,0,0,0}, Cardinal::north());
            success &= util.setupTogglerOrange(int4{0,0,0,0}, Cardinal::east());

            const auto createToggler = [&](const int4& coord){
                int outCharacterId, outFloorId;
                codeset.addFailure(!(success &= util.setupTogglerSwitch(coord, outCharacterId, outFloorId)));
                codeset.addFailure(!(success &= outCharacterId != -1), CODE_NEW_CHARACTER_BUT_BAD_ID);
            };
            createToggler(int4{0,0,0,0});
        });
    });

    if (!success) {
        return false;
    }

    return true;
}