#include "Codeset.hpp"
#include "DoorEnum.hpp"
#include "Dungeon.hpp"
#include "DungeonAuthor.hpp"
#include "DungeonGenerator.hpp"
#include "GeneratorDoorwayDungeon2.hpp"
#include "iLayout.hpp"
#include "LayoutEnum.hpp"
#include "LayoutFlyweight.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Pathfinder.hpp"
#include "RemodelEnum.hpp"
#include "RoleEnum.hpp"

bool GeneratorDoorwayDungeon2::generate (int seed, Match& dst, Codeset& codeset) const {
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
    });

    LayoutFlyweight::getFlyweights().accessConst(LAYOUT, [&](const LayoutFlyweight& flyweight){
        flyweight.layout.accessConst([&](const iLayout& layoutIntf){
            layoutIntf.setupAdjacencyPointers(dst.dungeon.rooms);
            DungeonAuthor util(controller, layoutIntf);

            int z;
            // setup all them walls/ladders
            constexpr int w = DOOR_WALL;
            constexpr int o = DOOR_DOORWAY;

            z = 6;
            util.setupHorizontalWalls({o, o, o}, 1, z);
            util.setupHorizontalWalls({o, o, o}, 0, z);
            z = 5;
            util.setupHorizontalWalls({o, o, o}, 1, z);
            util.setupHorizontalWalls({o, o, o}, 0, z);
            z = 4;
            util.setupHorizontalWalls({w, o, w}, 1, z);
            util.setupHorizontalWalls({w, o, w}, 0, z);
            z = 3; 
            util.setupHorizontalWalls({o, w, o}, 1, z);
            util.setupHorizontalWalls({o, o, o}, 0, z);
            z = 2;
            util.setupHorizontalWalls({o, o, o}, 1, z);
            util.setupHorizontalWalls({o, w, o}, 0, z);
            z = 1;
            util.setupHorizontalWalls({w, w, w}, 1, z);
            util.setupHorizontalWalls({w, w, w}, 0, z);
            z = 0;
            util.setupHorizontalWalls({w, w, w}, 1, z);
            util.setupHorizontalWalls({w, w, w}, 0, z);

            z = 6;
            util.setupVerticalWalls({w, w}, 2, z);
            util.setupVerticalWalls({w, w}, 1, z);
            util.setupVerticalWalls({o, o}, 0 , z);
            z = 5;
            util.setupVerticalWalls({w, w}, 2, z);
            util.setupVerticalWalls({o, o}, 1, z);
            util.setupVerticalWalls({w, w}, 0, z);
            z = 4;
            util.setupVerticalWalls({o, o}, 2, z);
            util.setupVerticalWalls({o, o}, 1, z);
            util.setupVerticalWalls({o, o}, 0, z);
            z = 3;
            util.setupVerticalWalls({o, o}, 2, z);
            util.setupVerticalWalls({w, w}, 1, z);
            util.setupVerticalWalls({o, o}, 0, z);
            z = 2;
            util.setupVerticalWalls({o, w}, 2, z);
            util.setupVerticalWalls({w, o}, 1, z);
            util.setupVerticalWalls({o, o}, 0, z);
            z = 1; 
            util.setupVerticalWalls({o, w}, 2, z);
            util.setupVerticalWalls({o, o}, 1, z);
            util.setupVerticalWalls({o, o}, 0, z);
            z = 0;
            util.setupVerticalWalls({o, o}, 2, z);
            util.setupVerticalWalls({o, o}, 1, z);
            util.setupVerticalWalls({o, o}, 0, z);
        });
    });

    if (seed == SKIP_SEED) {
        return success;
    }
    
    DungeonGenerator generator(playerId, builderId, BOSS_ROOM_ID, codeset);
    // generator.allowlist = Bitstick<REMODEL_COUNT>({REMODEL_TOGGLER_BLUE, REMODEL_TOGGLER_ORANGE, REMODEL_TOGGLER_SWITCH});
    Maybe<Match> result = generator.remodelLoop(dst, 2);
    if (codeset.addFailure(result.isEmpty())) {
        return false;
    }
    result.accessConst([&](const Match& created){
        dst = created;
    });

    return success;
}