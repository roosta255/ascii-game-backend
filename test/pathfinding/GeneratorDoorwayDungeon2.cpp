#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include "Cardinal.hpp"
#include "Character.hpp"
#include "CharacterAction.hpp"
#include "CodeEnum.hpp"
#include "Codeset.hpp"
#include "DoorEnum.hpp"
#include "Dungeon.hpp"
#include "GeneratorDoorwayDungeon2.hpp"
#include "GeneratorEnum.hpp"
#include "int2.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Pathfinder.hpp"
#include "PathfindingCounter.hpp"
#include "RoleEnum.hpp"
#include "Room.hpp"
#include "TestController.hpp"

TEST_CASE("Builder can pathfind to room #52", "[match][GENERATOR_DOORWAY_DUNGEON_2]") {
    TestController controller(GENERATOR_DOORWAY_DUNGEON_2);
    controller.generate(0);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);
    REQUIRE(controller.match.start());

    const auto builderOffset = controller.builderOffset;

    int steps = 0;
    const bool isFound = controller.controller.findCharacterPath(
        controller.BUILDER_ID, builderOffset, 1000,
        [&](const Match& match) {
            Match copy = match;
            MatchController check(copy, controller.codeset);
            bool inRoom = false;
            check.isCharacterWithinRoom(builderOffset, 52, inRoom);
            return inRoom;
        },
        [&](const CharacterAction&, const Match&) {
            return 0;
        },
        [&](const CharacterAction&, const Match&) {
            steps++;
        }
    );

    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(isFound);
}
