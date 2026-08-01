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
#include "CodesetExpect.hpp"
#include "TestController.hpp"

TEST_CASE("Test Time Gate", "[match][test4d]") {
    // Setup
    TestController controller(GENERATOR_TEST_4D);

    // Generate test layout
    controller.generate(0);
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    // Start the match
    REQUIRE(controller.match.start());

    controller.moveCharacterToWall(Cardinal::east());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    REQUIRE(controller.inventory.cubes == 0);
    REQUIRE(controller.inventory.isCubeAwakened == false);
    REQUIRE(controller.inventory.isCubeDormant == false);

    controller.moveCharacterToWall(Cardinal::north());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    // take dormant cube
    controller.activateLock(Cardinal::north());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    REQUIRE(controller.inventory.cubes == 1);
    REQUIRE(controller.inventory.isCubeAwakened == false);
    REQUIRE(controller.inventory.isCubeDormant);

    controller.endTurn();
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.moveCharacterToWall(Cardinal::east());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.moveCharacterToWall(Cardinal::north());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.endTurn();
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    // climb up ladder
    controller.activateDoor(Cardinal::east());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    controller.latestPosition = 17;

    // set dormant cube into lightning-rod
    controller.activateLock(Cardinal::north());
    // REQUIRE(controller.codeset.describe() == "");
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    REQUIRE(controller.inventory.cubes == 0);
    REQUIRE(controller.inventory.isCubeAwakened == false);
    REQUIRE(controller.inventory.isCubeDormant == false);

    controller.endTurn();
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    // take awakened cube
    controller.activateLock(Cardinal::north());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    REQUIRE(controller.inventory.cubes == 1);
    REQUIRE(controller.inventory.isCubeAwakened);
    REQUIRE(controller.inventory.isCubeDormant == false);

    // climb down pole
    controller.activateDoor(Cardinal::west());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    controller.latestPosition = 8;

    controller.endTurn();
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.moveCharacterToFloor(2);
    // REQUIRE(controller.codeset.describe() == "");
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.moveCharacterToWall(Cardinal::south());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    
    controller.endTurn();
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.moveCharacterToWall(Cardinal::west());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    
    // set awakened cube into time-gate
    controller.activateLock(Cardinal::north());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    REQUIRE(controller.inventory.cubes == 0);
    REQUIRE(controller.inventory.isCubeAwakened == false);
    REQUIRE(controller.inventory.isCubeDormant == false);

    // time travel to future
    controller.activateDoor(Cardinal::north());
    // hardcode the new roomId in the future
    controller.latestPosition = 22;
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    // take dormant cube from time-gate
    controller.activateLock(Cardinal::north());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    REQUIRE(controller.inventory.cubes == 1);
    REQUIRE(controller.inventory.isCubeAwakened == false);
    REQUIRE(controller.inventory.isCubeDormant);

    controller.endTurn();
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.moveCharacterToWall(Cardinal::west());
    // REQUIRE(controller.codeset.describe("") == "");
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    const Match restored = controller.saveAndLoadMatch();
    REQUIRE(restored == controller.match);
}
