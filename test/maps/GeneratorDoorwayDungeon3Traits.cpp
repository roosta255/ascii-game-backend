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
#include "CodesetExpect.hpp"
#include "TestController.hpp"
#include "TraitEnum.hpp"

TEST_CASE("DoorwayDungeon3Traits sequence completion", "[match][GENERATOR_DOORWAY_DUNGEON_3_TRAITS]") {
    TestController controller(GENERATOR_DOORWAY_DUNGEON_3_TRAITS);

    controller.generate(0);
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    REQUIRE(controller.match.start());

    REQUIRE(controller.latestPosition == GeneratorDoorwayDungeon3Traits::ENTRANCE_ROOM_ID);

    controller.moveCharacterToWall(Cardinal::north());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.activateObjectCharacter(ROLE_SACRAMENT_FORGIVENESS);
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    REQUIRE(controller.builderCharacterPtr->traitsAfflicted[(int)TRAIT_PIETY] == true);

    controller.endTurn();
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.activateLock(Cardinal::west());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.moveCharacterToWall(Cardinal::west());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.endTurn();
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.lootInventory(ROLE_CHEST, ITEM_KEY);
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    REQUIRE(controller.builderCharacterPtr->traitsAfflicted[(int)TRAIT_CAT_BITE] == true);
    REQUIRE(controller.controller.getTraitsComputed(controller.builderOffset).final[(int)TRAIT_PIETY] == false);
    REQUIRE(controller.inventory.keys == 1);

    controller.moveCharacterToFloor(2);
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.endTurn();
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.moveCharacterToWall(Cardinal::east());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.activateObjectCharacter(ROLE_SACRAMENT_FORGIVENESS);
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    REQUIRE(controller.builderCharacterPtr->traitsAfflicted[(int)TRAIT_PIETY] == true);
    REQUIRE(controller.builderCharacterPtr->traitsAfflicted[(int)TRAIT_CAT_BITE] == false);

    controller.endTurn();
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.moveCharacterToFloor(2);
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.endTurn();
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.moveCharacterToWall(Cardinal::west());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.endTurn();
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.activateLock(Cardinal::south());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.moveCharacterToWall(Cardinal::south());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.endTurn();
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.activateLock(Cardinal::east());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.moveCharacterToWall(Cardinal::east());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.endTurn();
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.activateLock(Cardinal::east());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.moveCharacterToWall(Cardinal::east());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    REQUIRE(controller.inventory.keys == 0);

    controller.endTurn();
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.activateObjectCharacter(ROLE_TOGGLER_BLUE);
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.endTurn();
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.moveCharacterToFloor(2);
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.activateLock(Cardinal::west());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    REQUIRE(controller.inventory.keys == 1);

    controller.endTurn();
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.activateLock(Cardinal::north());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.moveCharacterToWall(Cardinal::north());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.endTurn();
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.moveCharacterToWall(Cardinal::west());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.endTurn();
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.activateLock(Cardinal::west());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.moveCharacterToWall(Cardinal::west());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.endTurn();
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.moveCharacterToWall(Cardinal::north());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.endTurn();
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.activateLock(Cardinal::east());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.moveCharacterToWall(Cardinal::east());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.endTurn();
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.activateDoor(Cardinal::north());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    REQUIRE(controller.builderCharacterPtr->location.type == LOCATION_EXITED_DUNGEON);
}
