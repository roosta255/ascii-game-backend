#include <catch2/catch_test_macros.hpp>
#include "ActionEnum.hpp"
#include "Cardinal.hpp"
#include "GeneratorEnum.hpp"
#include "ItemEnum.hpp"
#include "LockEnum.hpp"
#include "Match.hpp"
#include "Preactivation.hpp"
#include "RoleEnum.hpp"
#include "TestController.hpp"

TEST_CASE("ChestKeyTutorial sequence completion", "[match][GENERATOR_CHEST_KEY_TUTORIAL]") {
    TestController controller(GENERATOR_CHEST_KEY_TUTORIAL);

    controller.generate(0);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    REQUIRE(controller.match.start());

    controller.updateInventory();
    REQUIRE(controller.inventory.isEmpty);
    REQUIRE(controller.inventory.keys == 0);

    // Move north from start room into room above
    controller.moveCharacterToWall(Cardinal::north());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.moveCharacterToFloor(4);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // Move north again into the top-center room
    controller.moveCharacterToWall(Cardinal::north());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.moveCharacterToFloor(4);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // Take key from the west keeper (keyed initially)
    controller.activateLock(Cardinal::west());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);
    REQUIRE(controller.inventory.keys == 1);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // Move south back into center room
    controller.moveCharacterToWall(Cardinal::south());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.moveCharacterToFloor(4);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // Give key to LOCK_KEY_KEEPER_CLOSED chest — chest unlocks
    controller.activateChestLock(LOCK_KEY_KEEPER_CLOSED);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);
    REQUIRE(controller.inventory.keys == 0);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // Loot ITEM_KEY from the now-open keeper chest
    controller.lootInventory(ROLE_CHEST, ITEM_KEY);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);
    REQUIRE(controller.inventory.keys == 1);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // Take key back from LOCK_KEY_KEEPER_OPEN chest — chest re-locks
    controller.activateChestLock(LOCK_KEY_KEEPER_OPEN);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);
    REQUIRE(controller.inventory.keys == 2);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // Move west into the left-center room
    controller.moveCharacterToWall(Cardinal::west());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.moveCharacterToFloor(4);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // Give key to the south keeper (keyless initially) to open the path south
    controller.activateLock(Cardinal::south());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);
    REQUIRE(controller.inventory.keys == 1);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // Move south through the now-keyed keeper into the bottom-left room
    controller.moveCharacterToWall(Cardinal::south());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.moveCharacterToFloor(4);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // Give key to LOCK_KEY_JAILER_CLOSED chest — jailer chest unlocks (key cannot be retrieved)
    controller.activateChestLock(LOCK_KEY_JAILER_CLOSED);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);
    REQUIRE(controller.inventory.keys == 0);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // Activate the toggler switch contained within the jailer chest
    controller.activateContainedCharacter(LOCK_KEY_JAILER_OPEN, ROLE_TOGGLER_BLUE);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);
    REQUIRE(controller.match.dungeon.isBlueOpen);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // Move north back through the keeper (now keyed) into the left-center room
    controller.moveCharacterToWall(Cardinal::north());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.moveCharacterToFloor(4);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // Retrieve the key from the south keeper to re-lock the path
    controller.activateLock(Cardinal::south());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);
    REQUIRE(controller.inventory.keys == 1);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // Move east into the center room
    controller.moveCharacterToWall(Cardinal::east());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // Move north into the top-center room
    controller.moveCharacterToWall(Cardinal::north());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // Insert the key from the west keeper to un-lock the path
    controller.activateLock(Cardinal::west());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // Move west into the top-left room (toggler blue chest is now open)
    controller.moveCharacterToWall(Cardinal::west());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.moveCharacterToFloor(4);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // Loot ITEM_KEY_ELEVATOR from the now-open toggler blue chest
    controller.lootInventory(ROLE_CHEST, ITEM_KEY_ELEVATOR);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // Move south back into the top-center room
    controller.moveCharacterToWall(Cardinal::east());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.moveCharacterToFloor(4);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

}

TEST_CASE("Monkey steals ITEM_FOOD from builder in shared room", "[match][GENERATOR_CHEST_KEY_TUTORIAL][monkey]") {
    TestController tc(GENERATOR_CHEST_KEY_TUTORIAL);
    tc.isSkippingAnimations = true;

    tc.generate(0);
    REQUIRE(tc.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(tc.match.start());

    // Seed builder with food
    tc.giveItem(ITEM_FOOD);

    // Move builder north into the monkey's starting room
    tc.moveCharacterToWall(Cardinal::north());
    REQUIRE(tc.isSuccess);
    tc.moveCharacterToFloor(4);
    REQUIRE(tc.isSuccess);
    const int monkeyRoomId = tc.latestPosition;

    // Exhaust builder's action budget so TRAIT_ACTION_READY becomes false,
    // which is the condition that makes a builder pickpocketable.
    tc.builderCharacterPtr->actions = 2;
    tc.controller.updateTraits(*tc.builderCharacterPtr);

    // Unlock the chest in the monkey's room so it can receive the stolen item.
    for (auto& chest : tc.match.dungeon.chests) {
        if (chest.containerCharacterId < 0) continue;
        bool inRoom = false;
        tc.controller.isCharacterWithinRoom(chest.containerCharacterId, monkeyRoomId, inRoom);
        if (!inRoom) continue;
        chest.lock = LOCK_NIL;
        break;
    }

    // Locate the monkey NPC
    int monkeyCharId    = -1;
    int monkeyLocRoomId = -1;
    for (auto& ch : tc.match.dungeon.characters) {
        if (ch.role == ROLE_MONKEY) {
            monkeyCharId    = ch.characterId;
            monkeyLocRoomId = ch.location.roomId;
            break;
        }
    }
    REQUIRE(monkeyCharId != -1);
    REQUIRE(monkeyLocRoomId == monkeyRoomId);

    // Fire the monkey's NPC action — it should pickpocket the food
    tc.isSuccess = tc.controller.activate(Preactivation{
        .action = {
            .type        = ACTION_NPC_ACT,
            .characterId = monkeyCharId,
            .roomId      = monkeyLocRoomId,
        },
        .playerId             = TestController::BUILDER_ID,
        .isSkippingAnimations = true,
    });

    REQUIRE(tc.isSuccess);

    // Food must have moved out of the builder's inventory
    int foodAfter = 0;
    tc.playerPtr->getInventory(tc.match.dungeon).accessItem(ITEM_FOOD, [&](auto& item) { foodAfter = item.stacks; });
    REQUIRE(foodAfter == 0);
}
