#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include "Cardinal.hpp"
#include "Character.hpp"
#include "CharacterAction.hpp"
#include "CodeEnum.hpp"
#include "Codeset.hpp"
#include "DoorEnum.hpp"
#include "Dungeon.hpp"
#include "GeneratorDoorwayDungeon1.hpp"
#include "GeneratorEnum.hpp"
#include "int2.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Pathfinder.hpp"
#include "PathfindingCounter.hpp"
#include "Remodel.hpp"
#include "RemodelKeeper.hpp"
#include "RemodelTogglerBlue.hpp"
#include "RemodelTogglerSwitch.hpp"
#include "RoleEnum.hpp"
#include "Room.hpp"
#include "TestController.hpp"

TEST_CASE("Verify empty dungeon is navigable manually", "[match][GENERATOR_DOORWAY_DUNGEON_1]") {
    TestController controller(GENERATOR_DOORWAY_DUNGEON_1);

    // Generate test layout
    controller.generate(GeneratorDoorwayDungeon1::SKIP_SEED);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());

    // Start the match
    REQUIRE(controller.match.start());

    const auto validateMovement = [&](Cardinal dir){
        controller.moveCharacterToWall(dir);
        REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
        REQUIRE(controller.isSuccess);

        controller.endTurn();
        REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
        REQUIRE(controller.isSuccess);
    };

    validateMovement(Cardinal::east());
    validateMovement(Cardinal::north());
    validateMovement(Cardinal::north());
    validateMovement(Cardinal::north());
    validateMovement(Cardinal::west());

    // climb down ladder
    controller.activateDoor(Cardinal::north());

    const Match restored = controller.saveAndLoadMatch();
    REQUIRE(restored == controller.match);
    /*
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);
    controller.latestPosition = 37;

    validateMovement(Cardinal::east());
    validateMovement(Cardinal::east());
    validateMovement(Cardinal::south());
    validateMovement(Cardinal::south());
    validateMovement(Cardinal::south());
    validateMovement(Cardinal::west());
    validateMovement(Cardinal::west());
    validateMovement(Cardinal::north());

    // climb down ladder
    controller.activateDoor(Cardinal::north());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);
    controller.latestPosition = 7;

    validateMovement(Cardinal::east());
    validateMovement(Cardinal::north());
    validateMovement(Cardinal::north());
    validateMovement(Cardinal::west());
    validateMovement(Cardinal::north());
    REQUIRE(controller.latestPosition == GeneratorDoorwayDungeon1::BOSS_ROOM_ID);
    */
}
