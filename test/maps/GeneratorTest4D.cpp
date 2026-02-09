#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include "ActionEnum.hpp"
#include "CharacterAction.hpp"
#include "Codeset.hpp"
#include "DoorEnum.hpp"
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
    controller.isSuccess = controller.controller.generate(0);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

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

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // climb up ladder
    controller.activateDoor(Cardinal::east());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);
    controller.latestPosition = 17;

    // set dormant cube into lightning-rod
    controller.activateLock(Cardinal::north());
    // REQUIRE(controller.codeset.describe() == "");
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);
    REQUIRE(controller.inventory.cubes == 0);
    REQUIRE(controller.inventory.isCubeAwakened == false);
    REQUIRE(controller.inventory.isCubeDormant == false);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // take awakened cube
    controller.activateLock(Cardinal::north());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);
    REQUIRE(controller.inventory.cubes == 1);
    REQUIRE(controller.inventory.isCubeAwakened);
    REQUIRE(controller.inventory.isCubeDormant == false);

    // climb down pole
    controller.activateDoor(Cardinal::west());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);
    controller.latestPosition = 8;

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

TEST_CASE("Check A* can complete match[test4d]", "[match][test4d]") {
    TestController controller(GENERATOR_TEST_4D);

    // Generate test layout
    controller.controller.generate(0);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());

    // Start the match
    REQUIRE(controller.match.start());

    const auto builderOffset = controller.builderOffset;
    controller.codeset.setTable(CODE_BUILDER_CHARACTER_ID, builderOffset);

    int moves = 0;
    constexpr static auto EXPECTED_ACTIONS = 12;
    Array<CharacterAction, EXPECTED_ACTIONS> actualActions;
    const bool isFound = controller.controller.findCharacterPath(controller.BUILDER_ID, builderOffset, 400, 
        [&](const Match& inputMatch){
            Match copyMatch = inputMatch;
            MatchController controller2(copyMatch, controller.codeset);
            bool isSuccess = false;
            controller2.isCharacterWithinRoom(builderOffset, 22, isSuccess);
            return isSuccess;
        }, [&](const Match& match){
            return 0; // no heuristic
        },
        [&](const CharacterAction& action, const Match& match){
            actualActions.set(moves++, action);
        });
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    // REQUIRE(controller.codeset.describe() == "");
    REQUIRE(isFound);
    REQUIRE(moves == EXPECTED_ACTIONS);
    REQUIRE(actualActions == std::array<CharacterAction, EXPECTED_ACTIONS>
        { CharacterAction{.type=ACTION_MOVE_TO_DOOR, .characterId=builderOffset, .roomId=0, .direction=Cardinal::east() }
        , CharacterAction{.type=ACTION_MOVE_TO_DOOR, .characterId=builderOffset, .roomId=1, .direction=Cardinal::north() }
        , CharacterAction{.type=ACTION_ACTIVATE_LOCK, .characterId=builderOffset, .roomId=4, .direction=Cardinal::north() }
        , CharacterAction{.type=ACTION_MOVE_TO_DOOR, .characterId=builderOffset, .roomId=4, .direction=Cardinal::east() }
        , CharacterAction{.type=ACTION_MOVE_TO_DOOR, .characterId=builderOffset, .roomId=5, .direction=Cardinal::north() }
        , CharacterAction{.type=ACTION_ACTIVATE_DOOR, .characterId=builderOffset, .roomId=8, .direction=Cardinal::east() }
        , CharacterAction{.type=ACTION_ACTIVATE_LOCK, .characterId=builderOffset, .roomId=17, .direction=Cardinal::north() }
        , CharacterAction{.type=ACTION_ACTIVATE_LOCK, .characterId=builderOffset, .roomId=17, .direction=Cardinal::north() }
        , CharacterAction{.type=ACTION_MOVE_TO_DOOR, .characterId=builderOffset, .roomId=8, .direction=Cardinal::south() }
        , CharacterAction{.type=ACTION_MOVE_TO_DOOR, .characterId=builderOffset, .roomId=5, .direction=Cardinal::west() }
        , CharacterAction{.type=ACTION_ACTIVATE_LOCK, .characterId=builderOffset, .roomId=4, .direction=Cardinal::north() }
        , CharacterAction{.type=ACTION_ACTIVATE_DOOR, .characterId=builderOffset, .roomId=4, .direction=Cardinal::north() }
        });
}
