#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include "Codeset.hpp"
#include "InventoryDigest.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
/*
#include "GeneratorEnum.hpp"
#include "GeneratorTutorial.hpp"
#include "Cardinal.hpp"
#include "Dungeon.hpp"
#include "Room.hpp"
#include "Character.hpp"
#include "CodeEnum.hpp"
#include "Match.hpp"
#include "RoleEnum.hpp"*/
#include "TestController.hpp"

TEST_CASE("Test Time Gate", "[match][test4d]") {
    // Setup
    TestController controller(GENERATOR_TEST_4D);

    // Generate test layout
    controller.controller.generate(0);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());

    // Start the match
    REQUIRE(controller.match.start());

    controller.moveCharacterToWall(Cardinal::east());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);
    REQUIRE(controller.inventory.cubes == 0);
    REQUIRE(controller.inventory.isCubeAwakened == false);
    REQUIRE(controller.inventory.isCubeDormant == false);

    controller.moveCharacterToWall(Cardinal::north());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // take dormant cube
    controller.activateLock(Cardinal::north());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);
    REQUIRE(controller.inventory.cubes == 1);
    REQUIRE(controller.inventory.isCubeAwakened == false);
    REQUIRE(controller.inventory.isCubeDormant);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.moveCharacterToWall(Cardinal::east());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.moveCharacterToWall(Cardinal::north());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // set dormant cube into lightning-rod
    controller.activateLock(Cardinal::north());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);
    REQUIRE(controller.inventory.cubes == 0);
    REQUIRE(controller.inventory.isCubeAwakened == false);
    REQUIRE(controller.inventory.isCubeDormant == false);

    // take awakened cube
    controller.activateLock(Cardinal::north());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);
    REQUIRE(controller.inventory.cubes == 1);
    REQUIRE(controller.inventory.isCubeAwakened);
    REQUIRE(controller.inventory.isCubeDormant == false);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.moveCharacterToFloor(2);
    // REQUIRE(controller.codeset.describe() == "");
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.moveCharacterToWall(Cardinal::south());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);
    
    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.moveCharacterToWall(Cardinal::west());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);
    
    // set awakened cube into time-gate
    controller.activateLock(Cardinal::north());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);
    REQUIRE(controller.inventory.cubes == 0);
    REQUIRE(controller.inventory.isCubeAwakened == false);
    REQUIRE(controller.inventory.isCubeDormant == false);

    // time travel to future
    controller.activateDoor(Cardinal::north());
    // hardcode the new roomId in the future
    controller.latestPosition = 22;
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // take dormant cube from time-gate
    controller.activateLock(Cardinal::north());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);
    REQUIRE(controller.inventory.cubes == 1);
    REQUIRE(controller.inventory.isCubeAwakened == false);
    REQUIRE(controller.inventory.isCubeDormant);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.moveCharacterToWall(Cardinal::west());
    // REQUIRE(controller.codeset.describe("") == "");
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);
    
}
