#include "Codeset.hpp"
#include "GeneratorUtility.hpp"
#include "DoorEnum.hpp"
#include "Dungeon.hpp"
#include "GeneratorTest4D.hpp"
#include "iLayout.hpp"
#include "LayoutEnum.hpp"
#include "LayoutFlyweight.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "RoleEnum.hpp"

bool GeneratorTest4D::generate (int seed, Match& dst, Codeset& codeset) const {
    MatchController controller(dst, codeset);
    CodeEnum& error = codeset.error;
    constexpr auto LAYOUT = LAYOUT_4D_3x3x2;
    dst.dungeon.layout = LAYOUT;
    bool success = true;
    
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

            util.setupTimeGateRoomToFuture(int4{1,1,0,0}, true, false);

            // this dungeon has 2 timezones
            for (int t = 0; t < 2; t++) {
                success &= util.setup2x5Room(int4{0,1,0,t})
                        && util.setup2x5Room(int4{2,1,0,t})
                        && util.setup4x1Room(int4{1,0,0,t})
                        && util.setup4x1Room(int4{1,2,0,t})
                        && util.setupLightningRodRoom(int4{2,2,1,t}, false, false)
                        && util.setupPoleUp(int4{2,2,0,t}, Cardinal::west())
                        && util.setupLadderUp(int4{2,2,0,t}, Cardinal::east())
                        && util.setupDoorway(int4{0,0,0,t}, Cardinal::east())
                        && util.setupDoorway(int4{0,1,0,t}, Cardinal::east())
                        && util.setupDoorway(int4{1,1,0,t}, Cardinal::east())
                        && util.setupDoorway(int4{1,0,0,t}, Cardinal::north())
                        && util.setupDoorway(int4{2,0,0,t}, Cardinal::north())
                        && util.setupDoorway(int4{2,1,0,t}, Cardinal::north());
            }

            // success &= util.setupDoorway(int4{0,2,0,1}, Cardinal::east());
            // success &= util.setupDoorway(int4{1,2,0,1}, Cardinal::east());

        });
    });

    return success;
}