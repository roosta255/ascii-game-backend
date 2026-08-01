#include <catch2/catch_test_macros.hpp>
#include "ActionEnum.hpp"
#include "BehaviorEnum.hpp"
#include "Cardinal.hpp"
#include "ConductEnum.hpp"
#include "ConductExpect.hpp"
#include "ConductMemory.hpp"
#include "ConductMemoryVariableEnum.hpp"
#include "GeneratorEnum.hpp"
#include "ItemEnum.hpp"
#include "LockEnum.hpp"
#include "Match.hpp"
#include "Preactivation.hpp"
#include "RoleEnum.hpp"
#include "CodesetExpect.hpp"
#include "TestController.hpp"

TEST_CASE("ChestKeyTutorial sequence completion", "[match][GENERATOR_CHEST_KEY_TUTORIAL]") {
    TestController controller(GENERATOR_CHEST_KEY_TUTORIAL);

    controller.generate(0);
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    REQUIRE(controller.match.start());

    controller.updateInventory();
    REQUIRE(controller.inventory.isEmpty);
    REQUIRE(controller.inventory.keys == 0);

    // Move north from start room into room above
    controller.moveCharacterToWall(Cardinal::north());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.moveCharacterToFloor(4);
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.endTurn();
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    // Move north again into the top-center room
    controller.moveCharacterToWall(Cardinal::north());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.moveCharacterToFloor(4);
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.endTurn();
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    // Take key from the west keeper (keyed initially)
    controller.activateLock(Cardinal::west());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    REQUIRE(controller.inventory.keys == 1);

    controller.endTurn();
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    // Move south back into center room
    controller.moveCharacterToWall(Cardinal::south());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.moveCharacterToFloor(4);
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.endTurn();
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    // Give key to LOCK_KEY_KEEPER_CLOSED chest — chest unlocks
    controller.activateChestLock(LOCK_KEY_KEEPER_CLOSED);
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    REQUIRE(controller.inventory.keys == 0);

    controller.endTurn();
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    // Loot ITEM_KEY from the now-open keeper chest
    controller.lootInventory(ROLE_CHEST, ITEM_KEY);
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    REQUIRE(controller.inventory.keys == 1);

    controller.endTurn();
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    // Take key back from LOCK_KEY_KEEPER_OPEN chest — chest re-locks
    controller.activateChestLock(LOCK_KEY_KEEPER_OPEN);
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    REQUIRE(controller.inventory.keys == 2);

    controller.endTurn();
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    // Move west into the left-center room
    controller.moveCharacterToWall(Cardinal::west());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.moveCharacterToFloor(4);
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.endTurn();
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    // Give key to the south keeper (keyless initially) to open the path south
    controller.activateLock(Cardinal::south());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    REQUIRE(controller.inventory.keys == 1);

    controller.endTurn();
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    // Move south through the now-keyed keeper into the bottom-left room
    controller.moveCharacterToWall(Cardinal::south());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.moveCharacterToFloor(4);
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.endTurn();
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    // Give key to LOCK_KEY_JAILER_CLOSED chest — jailer chest unlocks (key cannot be retrieved)
    controller.activateChestLock(LOCK_KEY_JAILER_CLOSED);
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    REQUIRE(controller.inventory.keys == 0);

    controller.endTurn();
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    // Activate the toggler switch contained within the jailer chest
    controller.activateContainedCharacter(LOCK_KEY_JAILER_OPEN, ROLE_TOGGLER_BLUE);
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    REQUIRE(controller.match.dungeon.isBlueOpen);

    controller.endTurn();
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    // Move north back through the keeper (now keyed) into the left-center room
    controller.moveCharacterToWall(Cardinal::north());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.moveCharacterToFloor(4);
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.endTurn();
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    // Retrieve the key from the south keeper to re-lock the path
    controller.activateLock(Cardinal::south());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    REQUIRE(controller.inventory.keys == 1);

    controller.endTurn();
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    // Move east into the center room
    controller.moveCharacterToWall(Cardinal::east());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.endTurn();
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    // Move north into the top-center room
    controller.moveCharacterToWall(Cardinal::north());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.endTurn();
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    // Insert the key from the west keeper to un-lock the path
    controller.activateLock(Cardinal::west());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    // Move west into the top-left room (toggler blue chest is now open)
    controller.moveCharacterToWall(Cardinal::west());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.endTurn();
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.moveCharacterToFloor(4);
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.endTurn();
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.endTurn();
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    // Loot ITEM_KEY_ELEVATOR from the now-open toggler blue chest
    controller.lootInventory(ROLE_CHEST, ITEM_KEY_ELEVATOR);
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.endTurn();
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    // Move south back into the top-center room
    controller.moveCharacterToWall(Cardinal::east());
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.moveCharacterToFloor(4);
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    controller.endTurn();
    REQUIRE_THAT(controller.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

}
