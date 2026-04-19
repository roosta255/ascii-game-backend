#include <catch2/catch_test_macros.hpp>
#include "Cardinal.hpp"
#include "Character.hpp"
#include "CharacterAction.hpp"
#include "GeneratorDoorwayDungeon3Traits.hpp"
#include "GeneratorEnum.hpp"
#include "ItemEnum.hpp"
#include "LocationEnum.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "RoleEnum.hpp"
#include "TestController.hpp"
#include "TraitEnum.hpp"

TEST_CASE("Check A* can exit DoorwayDungeon3Traits", "[match][GENERATOR_DOORWAY_DUNGEON_3_TRAITS]") {
    TestController controller(GENERATOR_DOORWAY_DUNGEON_3_TRAITS);
    return;

    controller.generate(0);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    REQUIRE(controller.match.start());

    const auto builderOffset = controller.builderOffset;

    int moves = 0;
    const bool isFound = controller.controller.findCharacterPath(controller.BUILDER_ID, builderOffset, 500,
        [&](const Match& match){
            bool isSuccess = false;
            match.builders.getPointer(0).accessConst([&](const Builder& builder){
                isSuccess = builder.character.location.type == LOCATION_EXITED_DUNGEON;
            });
            return isSuccess;
        }, [&](const CharacterAction& action, const Match& match){
            return 0; // no heuristic
        },
        [&](const CharacterAction& action, const Match& match){
            moves++;
        });
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(isFound);
}
