#include "Codeset.hpp"
#include "DoorEnum.hpp"
#include "Dungeon.hpp"
#include "DungeonAuthor.hpp"
#include "GeneratorDoorwayDungeon3Traits.hpp"
#include "iLayout.hpp"
#include "LayoutEnum.hpp"
#include "LayoutFlyweight.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "RoleEnum.hpp"

bool GeneratorDoorwayDungeon3Traits::generate (int seed, Match& dst, Codeset& codeset) const {
    MatchController controller(dst, codeset);
    constexpr auto LAYOUT = LAYOUT_7_LEVEL_TOWER;
    dst.dungeon.layout = LAYOUT;
    bool success = true;

    CodeEnum error = CODE_UNKNOWN_ERROR;
    if (!dst.setupSingleBuilder(error))
        return false;

    // TODO: starting locations
    int floorId = 0, builderId;
    std::string playerId;
    dst.builders.access(0, [&](Builder& builder){
        playerId = builder.player.account.toString();
        builder.character.role = ROLE_BUILDER;
        if (dst.containsCharacter(builder.character, builderId)) {
            bool isBuilderFloorFree = controller.findFreeFloor(ENTRANCE_ROOM_ID, CHANNEL_CORPOREAL, floorId);
            if (isBuilderFloorFree) {
                controller.assignCharacterToFloor(builderId, ENTRANCE_ROOM_ID, CHANNEL_CORPOREAL, floorId);
            }
        }
        controller.updateTraits(builder.character);
    });

    LayoutFlyweight::getFlyweights().accessConst(LAYOUT, [&](const LayoutFlyweight& flyweight){
        flyweight.layout.accessConst([&](const iLayout& layoutIntf){
            layoutIntf.setupAdjacencyPointers(dst.dungeon.rooms);
            DungeonAuthor util(controller, layoutIntf);

            int z;
            // setup all them walls/ladders
            constexpr DoorEnum w = DOOR_WALL;
            constexpr DoorEnum d = DOOR_DOORWAY;
            constexpr DoorEnum c = DOOR_COVENANT_CLOSED;
            constexpr DoorEnum o = DOOR_TOGGLER_ORANGE_OPEN;
            constexpr DoorEnum b = DOOR_TOGGLER_BLUE_CLOSED;

            z = 6;
            util.setupHorizontalWalls({b, w, w}, 1, z);
            util.setupHorizontalWalls({c, o, c}, 0, z);

            z = 6;
            util.setupVerticalWalls({d, w}, 2, z);
            util.setupVerticalWalls({c, b}, 1, z);
            util.setupVerticalWalls({c, d}, 0 , z);

            success &= util.setupShifter(int4{0,2,6,0}, Cardinal::east(), false);
            success &= util.setupShifter(int4{1,0,6,0}, Cardinal::east(), false);
            success &= util.setupExitDoor(int4{1,2,6,0}, Cardinal::north());

            const auto createToggler = [&](const int4& coord){
                int outCharacterId = -1, outFloorId = -1;
                codeset.addFailure(!(success &= util.setupTogglerSwitch(coord, outCharacterId, outFloorId)));
                codeset.addFailure(!(success &= outCharacterId != -1), CODE_NEW_CHARACTER_BUT_BAD_ID);
            };
            createToggler(int4{2,0,6,0});

            success &= controller.allocateChest(57, [&](Chest& chest, Character& container, Character& critter) {
                container.role = ROLE_CHEST;
                critter.role = ROLE_CAT;
                chest.inventory.giveItem(ITEM_KEY, codeset.error);
            });

            int outCharacterId = -1, outFloorId = -1;
            codeset.addFailure(!(success &= util.setupSacramentForgiveness(int4{1,1,6,0}, outCharacterId, outFloorId)));
            codeset.addFailure(!(success &= util.setupSacramentForgiveness(int4{0,0,6,0}, outCharacterId, outFloorId)));
        });
    });

    return success;
}
