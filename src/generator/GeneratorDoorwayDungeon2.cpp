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
    constexpr auto LAYOUT = LAYOUT_3D_5x4x3;
    dst.dungeon.layout = LAYOUT;
    bool success = true;

    CodeEnum error = CODE_UNKNOWN_ERROR;
    if (!dst.setupSingleBuilder(error))
        return false;

    // TODO: starting locations
    int floorId = 0, builderId, startingRoomId = 42;
    std::string playerId;
    dst.builders.access(0, [&](Builder& builder){
        playerId = builder.player.account.toString();
        builder.character.role = ROLE_BUILDER;
        if (dst.containsCharacter(builder.character, builderId)) {
            bool isBuilderFloorFree = controller.findFreeFloor(startingRoomId, CHANNEL_CORPOREAL, floorId);
            if (isBuilderFloorFree) {
                controller.assignCharacterToFloor(builderId, startingRoomId, CHANNEL_CORPOREAL, floorId);
            }
        }
    });

    LayoutFlyweight::getFlyweights().accessConst(LAYOUT, [&](const LayoutFlyweight& flyweight){
        flyweight.layout.accessConst([&](const iLayout& layoutIntf){
            layoutIntf.setupAdjacencyPointers(dst.dungeon.rooms);
            DungeonAuthor util(controller, layoutIntf);

            
            // setup all them walls/ladders
            constexpr int w = DOOR_WALL;
            constexpr int o = DOOR_DOORWAY;

            // z: 0
            util.setupHorizontalWalls({w, o, o, o, w}, 2, 0);
            util.setupHorizontalWalls({w, o, w, o, w}, 1, 0);
            util.setupHorizontalWalls({w, o, o, o, w}, 0, 0);
            // z: 1
            util.setupHorizontalWalls({o, o, o, o, o}, 2, 1);
            util.setupHorizontalWalls({o, w, w, w, o}, 1, 1);
            util.setupHorizontalWalls({o, o, o, o, o}, 0, 1);
            // z: 2
            util.setupHorizontalWalls({o, o, o, o, o}, 2, 2);
            util.setupHorizontalWalls({w, o, w, o, w}, 1, 2);
            util.setupHorizontalWalls({o, o, o, o, o}, 0, 2);

            // z: 0
            util.setupVerticalWalls({o, w, w, o}, 3, 0);
            util.setupVerticalWalls({o, w, o, o}, 2, 0);
            util.setupVerticalWalls({o, o, o, o}, 1, 0);
            util.setupVerticalWalls({o, w, w, o}, 0, 0);
            // z: 1
            util.setupVerticalWalls({o, o, o, o}, 3, 1);
            util.setupVerticalWalls({w, w, w, w}, 2, 1);
            util.setupVerticalWalls({w, w, w, w}, 1, 1);
            util.setupVerticalWalls({o, o, o, o}, 0, 1);
            // z: 2
            util.setupVerticalWalls({w, o, o, w}, 3, 2);
            util.setupVerticalWalls({o, w, w, o}, 2, 2);
            util.setupVerticalWalls({o, w, w, o}, 1, 2);
            util.setupVerticalWalls({w, o, o, w}, 0, 2);

            util.setupLadderUp(int4{2,1,0,0}, Cardinal::north());
            util.setupLadderUp(int4{2,3,1,0}, Cardinal::north());

            if (seed == SKIP_SEED) {
                return;
            }
            
            /*
            // z: 0
            util.setupHorizontalWalls({o, o, o, o, o}, 2, 0);
            util.setupHorizontalWalls({o, o, o, o, o}, 1, 0);
            util.setupHorizontalWalls({o, o, w, o, o}, 0, 0);
            // z: 1
            util.setupHorizontalWalls({o, w, o, w, o}, 2, 1);
            util.setupHorizontalWalls({w, w, o, w, w}, 1, 1);
            util.setupHorizontalWalls({o, w, w, w, o}, 0, 1);
            // z: 2
            util.setupHorizontalWalls({w, o, w, o, w}, 2, 2);
            util.setupHorizontalWalls({o, w, o, w, o}, 1, 2);
            util.setupHorizontalWalls({o, w, o, o, o}, 0, 2);

            // z: 0
            util.setupVerticalWalls({o, w, w, o}, 3, 0);
            util.setupVerticalWalls({w, o, o, w}, 2, 0);
            util.setupVerticalWalls({w, o, o, w}, 1, 0);
            util.setupVerticalWalls({o, o, o, o}, 0, 0);
            // z: 1
            util.setupVerticalWalls({o, o, o, o}, 3, 1);
            util.setupVerticalWalls({o, o, o, o}, 2, 1);
            util.setupVerticalWalls({o, o, o, o}, 1, 1);
            util.setupVerticalWalls({o, o, o, o}, 0, 1);
            // z: 2
            util.setupVerticalWalls({o, o, o, o}, 3, 2);
            util.setupVerticalWalls({o, w, w, o}, 2, 2);
            util.setupVerticalWalls({o, w, w, w}, 1, 2);
            util.setupVerticalWalls({o, o, o, o}, 0, 2);

            util.setupLadderUp(int4{2,3,1,0}, Cardinal::north());
            util.setupLadderUp(int4{2,0,0,0}, Cardinal::north());
            */

            // procedural generation starts
            // i give it a match, and it would return a different mutated match because these mutations could be recursive
            
        });
    });

    if (seed == SKIP_SEED) {
        return success;
    }
    
    DungeonGenerator generator(playerId, builderId, BOSS_ROOM_ID, codeset);
    // generator.allowlist = Bitstick<REMODEL_COUNT>({REMODEL_TOGGLER_BLUE, REMODEL_TOGGLER_ORANGE, REMODEL_TOGGLER_SWITCH});
    Maybe<Match> result = generator.remodelLoop(dst, 20);
    if (codeset.addFailure(result.isEmpty())) {
        return false;
    }
    result.accessConst([&](const Match& created){
        dst = created;
    });

    return success;
}