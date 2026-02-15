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

Array<int, DOOR_COUNT> countDoorsByType(const Match& match) {
    Array<int, DOOR_COUNT> results;
    for(const auto& room: match.dungeon.rooms) {
        for(const Cardinal dir: Cardinal::getAllCardinals()) {
            const auto doorType = room.getWall(dir).door;
            results.access(doorType, [&](int& count){
                count++;
            });
        }
    }
    return results;
}

Array<int, ROLE_COUNT> countCharactersByType(const Match& match) {
    Array<int, ROLE_COUNT> results;
    match.accessUsedCharacters([&](const Character& character){
        results.access(character.role, [&](int& count){
            count++;
        });
    });
    return results;
}

TEST_CASE("Test default generation", "[match][GENERATOR_DOORWAY_DUNGEON_1]") {
    TestController controller(GENERATOR_DOORWAY_DUNGEON_1);

    // Generate test layout
    controller.controller.generate(0);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());

    const auto path = Pathfinder::build(controller.BUILDER_ID, controller.builderOffset, GeneratorDoorwayDungeon1::BOSS_ROOM_ID, controller.match, controller.codeset);

    // sanity check
    REQUIRE(path.isSolved);

    // count types
    /*
    const auto actualDoorCounts = countDoorsByType(controller.match);
    std::array<int, DOOR_COUNT> expectedDoorCounts{};
    expectedDoorCounts[DOOR_WALL] = 134;
    expectedDoorCounts[DOOR_DOORWAY] = 110;
    expectedDoorCounts[DOOR_LADDER_1_BOTTOM] = 2;
    expectedDoorCounts[DOOR_LADDER_1_TOP] = 2;
    expectedDoorCounts[DOOR_TOGGLER_BLUE_CLOSED] = 2;
    REQUIRE(actualDoorCounts == expectedDoorCounts);

    const auto actualRoleCounts = countCharactersByType(controller.match);
    std::array<int, ROLE_COUNT> expectedRoleCounts{};
    expectedRoleCounts[ROLE_TOGGLER] = 1;
    expectedRoleCounts[ROLE_BUILDER] = 1;
    REQUIRE(actualRoleCounts == expectedRoleCounts);
    REQUIRE(controller.codeset.describe() == "");
    */
}

TEST_CASE("Verify empty dungeon is navigable manually", "[match][GENERATOR_DOORWAY_DUNGEON_1]") {
    TestController controller(GENERATOR_DOORWAY_DUNGEON_1);

    // Generate test layout
    controller.controller.generate(GeneratorDoorwayDungeon1::SKIP_SEED);
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

TEST_CASE("Test keeper generation", "[match][GENERATOR_DOORWAY_DUNGEON_1]") {
    TestController controller(GENERATOR_DOORWAY_DUNGEON_1);

    // Generate test layout
    controller.controller.generate(GeneratorDoorwayDungeon1::SKIP_SEED);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());

    

    // sanity check
    const auto pathBefore = Pathfinder::build(controller.BUILDER_ID, controller.builderOffset, GeneratorDoorwayDungeon1::BOSS_ROOM_ID, controller.match, controller.codeset);
    REQUIRE(pathBefore.isSolved);

    auto remodel = controller.buildRemodel(GeneratorDoorwayDungeon1::BOSS_ROOM_ID, [&](const Match& proposed, const PathfindingCounter&){
        auto actualDoorCounts = countDoorsByType(proposed);
        if (actualDoorCounts[DOOR_TOGGLER_BLUE_CLOSED].orElse(0) == 0) {
            return false;
        }

        auto actualRoleCounts = countCharactersByType(proposed);
        if (actualRoleCounts[ROLE_TOGGLER].orElse(0) == 0) {
            return false;
        }
        return true;
    });
    RemodelKeeper remodelKeeper;
    RemodelTogglerBlue remodelTogglerBlue;
    RemodelTogglerSwitch remodelTogglerSwitch;
    auto builtMatch = remodelTogglerBlue.buildMatch(remodel, controller.match);
    // REQUIRE(controller.codeset.describe()== "");
    REQUIRE(builtMatch.isPresent());

    // count types
    builtMatch.access([&](Match& match){
        const auto actualDoorCounts = countDoorsByType(match);
        std::array<int, DOOR_COUNT> expectedDoorCounts{};
        expectedDoorCounts[DOOR_WALL] = 134;
        expectedDoorCounts[DOOR_DOORWAY] = 116;
        expectedDoorCounts[DOOR_LADDER_1_BOTTOM] = 2;
        expectedDoorCounts[DOOR_LADDER_1_TOP] = 2;
        expectedDoorCounts[DOOR_TOGGLER_BLUE_CLOSED] = 2;
        REQUIRE(actualDoorCounts == expectedDoorCounts);

        const auto actualRoleCounts = countCharactersByType(match);
        std::array<int, ROLE_COUNT> expectedRoleCounts{};
        expectedRoleCounts[ROLE_TOGGLER] = 1;
        expectedRoleCounts[ROLE_BUILDER] = 1;
        REQUIRE(actualRoleCounts == expectedRoleCounts);

        // check whether new dungeon is solvable
        const auto pathAfter = Pathfinder::build(controller.BUILDER_ID, controller.builderOffset, GeneratorDoorwayDungeon1::BOSS_ROOM_ID, match, controller.codeset);
        REQUIRE(pathAfter.isSolved);
    });

}
