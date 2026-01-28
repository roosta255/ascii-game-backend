#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include "Codeset.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "GeneratorEnum.hpp"
#include "GeneratorTutorial.hpp"
#include "TestController.hpp"
#include "Cardinal.hpp"
#include "Dungeon.hpp"
#include "Room.hpp"
#include "Character.hpp"
#include "CodeEnum.hpp"
#include "Match.hpp"
#include "RoleEnum.hpp"

TEST_CASE("Test jailer", "[match][test]") {
    TestController controller(GENERATOR_TEST);

    // Generate test layout
    controller.controller.generate(0);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());

    // Start the match
    REQUIRE(controller.match.start());

    controller.updateInventory();
    REQUIRE(controller.inventory.isEmpty);
    REQUIRE(controller.inventory.keys == 0);

    controller.moveCharacterToWall(Cardinal::east());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // ----------- OLD CODE ------------
/*
    isSuccess = controller.moveCharacterToFloor(1, builderOffset, 1, Timestamp::nil());
    REQUIRE(codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(isSuccess);

    isSuccess = controller.moveCharacterToWall(1, builderOffset, Cardinal::north());
    REQUIRE(codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(isSuccess);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    isSuccess = controller.moveCharacterToWall(9, builderOffset, Cardinal::west());
    REQUIRE(result == CODE_BLOCKING_DOOR_TYPE);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    // Take keeper's key between rooms 9 and 8
    isSuccess = controller.activateLock(builderId, builderOffset, 9, Cardinal::west().getIndex(), result);
    REQUIRE(result == CODE_SUCCESS);
    REQUIRE(isSuccess == true);
    REQUIRE(inventory->makeDigest().isEmpty == false);
    REQUIRE(inventory->makeDigest().keys == 1);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    isSuccess = controller.moveCharacterToWall(9, builderOffset, Cardinal::north());
    REQUIRE(codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(isSuccess);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    isSuccess = controller.moveCharacterToWall(17, builderOffset, Cardinal::west());
    REQUIRE(result == CODE_BLOCKING_DOOR_TYPE);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    // Fill jailer between rooms 17 and 16
    isSuccess = controller.activateLock(builderId, builderOffset, 17, Cardinal::west().getIndex(), codeset);
    REQUIRE(result == CODE_SUCCESS);
    REQUIRE(isSuccess == true);
    REQUIRE(inventory->makeDigest().isEmpty == false);
    REQUIRE(inventory->makeDigest().keys == 1);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    isSuccess = controller.moveCharacterToWall(17, builderOffset, Cardinal::west());
    REQUIRE(codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(isSuccess);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);
*/
}
