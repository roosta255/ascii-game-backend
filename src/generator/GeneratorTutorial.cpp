#include "Codeset.hpp"
#include "GeneratorUtility.hpp"
#include "DoorEnum.hpp"
#include "Dungeon.hpp"
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
            GeneratorUtility util(dst.dungeon.rooms, layoutIntf, codeset);
            util.setupAdjacencyPointers();

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

        });
    });

    Character toggler;
    toggler.role = ROLE_TOGGLER;
    toggler.visibility = ~0x0;
    int outCharacterId;
    codeset.addFailure(!(success &= controller.addCharacterToFloor(toggler, 1, CHANNEL_CORPOREAL, outCharacterId)));
    codeset.addFailure(outCharacterId == -1, CODE_NEW_CHARACTER_BUT_BAD_ID);
    codeset.addFailure(!(success &= controller.addCharacterToFloor(toggler, 3, CHANNEL_CORPOREAL, outCharacterId)));
    codeset.addFailure(outCharacterId == -1, CODE_NEW_CHARACTER_BUT_BAD_ID);
    codeset.addFailure(!(success &= controller.addCharacterToFloor(toggler, 9, CHANNEL_CORPOREAL, outCharacterId)));
    codeset.addFailure(outCharacterId == -1, CODE_NEW_CHARACTER_BUT_BAD_ID);
    codeset.addFailure(!(success &= controller.addCharacterToFloor(toggler, 18, CHANNEL_CORPOREAL, outCharacterId)));
    codeset.addFailure(outCharacterId == -1, CODE_NEW_CHARACTER_BUT_BAD_ID);

    if (!success) {
        return false;
    }

    return true;
}