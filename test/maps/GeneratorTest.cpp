#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include "ActivatorChestLockKey.hpp"
#include "ActionEnum.hpp"
#include "Builder.hpp"
#include "Cardinal.hpp"
#include "Character.hpp"
#include "CharacterAction.hpp"
#include "Chest.hpp"
#include "CodeEnum.hpp"
#include "Codeset.hpp"
#include "Dungeon.hpp"
#include "GeneratorEnum.hpp"
#include "GeneratorTutorial.hpp"
#include "ItemEnum.hpp"
#include "LockEnum.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Preactivation.hpp"
#include "RoleEnum.hpp"
#include "Room.hpp"
#include "TestController.hpp"
#include "TraitEnum.hpp"

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

static int findChestContainerByLock(TestController& tc, LockEnum targetLock) {
    for (const Chest& chest : tc.match.dungeon.chests) {
        if (chest.lock == targetLock) return chest.containerCharacterId;
    }
    return -1;
}

TEST_CASE("Catalyst key chest: key not consumed, action taken", "[match][chest][lock]") {
    TestController tc(GENERATOR_TEST);

    tc.generate(0);
    REQUIRE(tc.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(tc.match.start());

    tc.giveItem(ITEM_KEY);
    REQUIRE(tc.inventory.keys == 1);

    int containerId = findChestContainerByLock(tc, LOCK_KEY_CATALYST_CLOSED);
    REQUIRE(containerId != -1);

    static ActivatorChestLockKey activator;
    tc.isSuccess = tc.controller.activate(activator, Preactivation{
        .action = {
            .characterId = tc.builderOffset,
            .roomId = tc.latestPosition,
            .targetCharacterId = containerId,
        },
        .playerId = TestController::BUILDER_ID,
    });
    tc.updateInventory();

    REQUIRE(tc.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(tc.isSuccess);
    REQUIRE(tc.inventory.keys == 1);                        // key NOT consumed
    REQUIRE(tc.builderCharacterPtr->actions == 1);          // one action taken
    REQUIRE(tc.builderCharacterPtr->traitsAfflicted[TRAIT_SNAKE_BITE].orElse(false)); // snake bit the builder

    int openContainerId = findChestContainerByLock(tc, LOCK_KEY_CATALYST_OPEN);
    REQUIRE(openContainerId == containerId);                 // lock transitioned to open

    const Match restored = tc.saveAndLoadMatch();
    REQUIRE(restored == tc.match);
}

TEST_CASE("Consumer key chest: key consumed, action taken", "[match][chest][lock]") {
    TestController tc(GENERATOR_TEST);

    tc.generate(0);
    REQUIRE(tc.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(tc.match.start());

    tc.giveItem(ITEM_KEY);
    REQUIRE(tc.inventory.keys == 1);

    int containerId = findChestContainerByLock(tc, LOCK_KEY_CONSUMER_CLOSED);
    REQUIRE(containerId != -1);

    static ActivatorChestLockKey activator;
    tc.isSuccess = tc.controller.activate(activator, Preactivation{
        .action = {
            .characterId = tc.builderOffset,
            .roomId = tc.latestPosition,
            .targetCharacterId = containerId,
        },
        .playerId = TestController::BUILDER_ID,
    });
    tc.updateInventory();

    REQUIRE(tc.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(tc.isSuccess);
    REQUIRE(tc.inventory.keys == 0);                        // key consumed
    REQUIRE(tc.builderCharacterPtr->actions == 1);          // one action taken
    REQUIRE(tc.builderCharacterPtr->traitsAfflicted[TRAIT_SNAKE_BITE].orElse(false)); // snake bit the builder

    int openContainerId = findChestContainerByLock(tc, LOCK_KEY_CONSUMER_OPEN);
    REQUIRE(openContainerId == containerId);                 // lock transitioned to open
}

TEST_CASE("Pathfinding: catalyst key chest USE_CHEST_LOCK action is found", "[match][chest][lock][pathfinding]") {
    TestController tc(GENERATOR_TEST);

    tc.generate(0);
    REQUIRE(tc.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(tc.match.start());

    tc.giveItem(ITEM_KEY);
    REQUIRE(tc.inventory.keys == 1);

    int containerId = findChestContainerByLock(tc, LOCK_KEY_CATALYST_CLOSED);
    REQUIRE(containerId != -1);

    int actionCount = 0;
    CharacterAction foundAction{};
    const bool isFound = tc.controller.findCharacterPath(
        TestController::BUILDER_ID, tc.builderOffset, 10,
        [&](const Match& match) {
            for (const Chest& chest : match.dungeon.chests) {
                if (chest.containerCharacterId == containerId && chest.lock == LOCK_KEY_CATALYST_OPEN) {
                    bool isSnakeBit = false;
                    match.builders.accessConst(TestController::BUILDER_INDEX, [&](const Builder& b) {
                        isSnakeBit = b.character.traitsAfflicted[TRAIT_SNAKE_BITE].orElse(false);
                    });
                    return isSnakeBit;
                }
            }
            return false;
        },
        [&](const CharacterAction&, const Match&) { return 0; },
        [&](const CharacterAction& action, const Match&) {
            foundAction = action;
            actionCount++;
        }
    );

    REQUIRE(tc.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(isFound);
    REQUIRE(actionCount == 1);
    REQUIRE(foundAction.type == ACTION_USE_CHEST_LOCK);
    REQUIRE(foundAction.targetCharacterId == containerId);
}

TEST_CASE("Pathfinding: consumer key chest USE_CHEST_LOCK action is found", "[match][chest][lock][pathfinding]") {
    TestController tc(GENERATOR_TEST);

    tc.generate(0);
    REQUIRE(tc.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(tc.match.start());

    tc.giveItem(ITEM_KEY);
    REQUIRE(tc.inventory.keys == 1);

    int containerId = findChestContainerByLock(tc, LOCK_KEY_CONSUMER_CLOSED);
    REQUIRE(containerId != -1);

    int actionCount = 0;
    CharacterAction foundAction{};
    const bool isFound = tc.controller.findCharacterPath(
        TestController::BUILDER_ID, tc.builderOffset, 10,
        [&](const Match& match) {
            for (const Chest& chest : match.dungeon.chests) {
                if (chest.containerCharacterId == containerId && chest.lock == LOCK_KEY_CONSUMER_OPEN) {
                    bool isSnakeBit = false;
                    match.builders.accessConst(TestController::BUILDER_INDEX, [&](const Builder& b) {
                        isSnakeBit = b.character.traitsAfflicted[TRAIT_SNAKE_BITE].orElse(false);
                    });
                    return isSnakeBit;
                }
            }
            return false;
        },
        [&](const CharacterAction&, const Match&) { return 0; },
        [&](const CharacterAction& action, const Match&) {
            foundAction = action;
            actionCount++;
        }
    );

    REQUIRE(tc.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(isFound);
    REQUIRE(actionCount == 1);
    REQUIRE(foundAction.type == ACTION_USE_CHEST_LOCK);
    REQUIRE(foundAction.targetCharacterId == containerId);
}
