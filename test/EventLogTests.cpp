#include <catch2/catch_test_macros.hpp>
#include "ActionEnum.hpp"
#include "ActivatorChestLockKey.hpp"
#include "Cardinal.hpp"
#include "Chest.hpp"
#include "Codeset.hpp"
#include "DoorEnum.hpp"
#include "Dungeon.hpp"
#include "EventEnum.hpp"
#include "GeneratorDoorwayDungeon2.hpp"
#include "GeneratorEnum.hpp"
#include "ItemEnum.hpp"
#include "LoggedEvent.hpp"
#include "LockEnum.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Preactivation.hpp"
#include "Room.hpp"
#include "RoleEnum.hpp"
#include "TestController.hpp"

// ---- helpers ----

static LoggedEvent requireLastEvent(Room& room) {
    auto log = room.getEventLog();
    REQUIRE(log.size() > 0);
    return log[log.size() - 1];
}

static bool findRoomWithDoor(TestController& tc, DoorEnum doorType, int& outRoomId, Cardinal& outDir) {
    for (Room& room : tc.match.dungeon.rooms) {
        for (int d = 0; d < 4; d++) {
            Cardinal dir = Cardinal(d);
            if (room.getWall(dir).door == doorType) {
                outRoomId = room.roomId;
                outDir = dir;
                return true;
            }
        }
    }
    return false;
}

// ---- tests ----

TEST_CASE("EventLog: MOVE_TO_DOOR", "[eventlog]") {
    TestController tc(GENERATOR_TEST);
    tc.isSkippingAnimations = true;
    tc.generate(0);
    REQUIRE(tc.match.start());

    const int startRoomId = tc.latestPosition;
    tc.moveCharacterToWall(Cardinal::east());
    REQUIRE(tc.isSuccess);

    tc.match.dungeon.rooms.access(startRoomId, [&](Room& room) {
        auto event = requireLastEvent(room);
        REQUIRE(event.action == EVENT_MOVE_TO_DOOR);
    });
}

TEST_CASE("EventLog: KEEPER_LOCK", "[eventlog]") {
    TestController tc(GENERATOR_TEST);
    tc.isSkippingAnimations = true;
    tc.generate(0);
    REQUIRE(tc.match.start());

    int roomId = -1;
    Cardinal dir;
    REQUIRE(findRoomWithDoor(tc, DOOR_KEEPER_INGRESS_KEYED, roomId, dir));

    tc.isSuccess = tc.controller.activate(Preactivation{
        .action = {
            .type = ACTION_ACTIVATE_LOCK,
            .characterId = tc.builderOffset,
            .roomId = roomId,
            .direction = dir,
        },
        .playerId = TestController::BUILDER_ID,
        .isSkippingAnimations = true,
    });
    REQUIRE(tc.isSuccess);

    tc.match.dungeon.rooms.access(roomId, [&](Room& room) {
        auto event = requireLastEvent(room);
        REQUIRE(event.action == EVENT_KEEPER_LOCK);
    });
}

TEST_CASE("EventLog: JAILER_LOCK", "[eventlog]") {
    TestController tc(GENERATOR_TEST);
    tc.isSkippingAnimations = true;
    tc.generate(0);
    REQUIRE(tc.match.start());

    tc.giveItem(ITEM_KEY);

    int roomId = -1;
    Cardinal dir;
    REQUIRE(findRoomWithDoor(tc, DOOR_JAILER_INGRESS_KEYLESS, roomId, dir));

    tc.isSuccess = tc.controller.activate(Preactivation{
        .action = {
            .type = ACTION_ACTIVATE_LOCK,
            .characterId = tc.builderOffset,
            .roomId = roomId,
            .direction = dir,
        },
        .playerId = TestController::BUILDER_ID,
        .isSkippingAnimations = true,
    });
    REQUIRE(tc.isSuccess);

    tc.match.dungeon.rooms.access(roomId, [&](Room& room) {
        auto event = requireLastEvent(room);
        REQUIRE(event.action == EVENT_JAILER_LOCK);
    });
}

TEST_CASE("EventLog: SHIFTER_LOCK", "[eventlog]") {
    TestController tc(GENERATOR_TUTORIAL);
    tc.isSkippingAnimations = true;
    tc.generate(0);
    REQUIRE(tc.match.start());

    tc.giveItem(ITEM_KEY);

    int roomId = -1;
    Cardinal dir;
    REQUIRE(findRoomWithDoor(tc, DOOR_SHIFTER_INGRESS_KEYLESS, roomId, dir));

    tc.isSuccess = tc.controller.activate(Preactivation{
        .action = {
            .type = ACTION_ACTIVATE_LOCK,
            .characterId = tc.builderOffset,
            .roomId = roomId,
            .direction = dir,
        },
        .playerId = TestController::BUILDER_ID,
        .isSkippingAnimations = true,
    });
    REQUIRE(tc.isSuccess);

    tc.match.dungeon.rooms.access(roomId, [&](Room& room) {
        auto event = requireLastEvent(room);
        REQUIRE(event.action == EVENT_SHIFTER_LOCK);
    });
}

TEST_CASE("EventLog: ELEVATOR", "[eventlog]") {
    TestController tc(GENERATOR_DOORWAY_DUNGEON_2);
    tc.isSkippingAnimations = true;
    tc.generate(0);
    REQUIRE(tc.match.start());

    tc.giveItem(ITEM_KEY_ELEVATOR);

    tc.isSuccess = tc.controller.activate(Preactivation{
        .action = {
            .type = ACTION_ACTIVATE_LOCK,
            .characterId = tc.builderOffset,
            .roomId = GeneratorDoorwayDungeon2::ELEVATOR_ROOM_ID,
            .direction = GeneratorDoorwayDungeon2::ELEVATOR_EXIT_DIRECTION,
        },
        .playerId = TestController::BUILDER_ID,
        .isSkippingAnimations = true,
    });
    REQUIRE(tc.isSuccess);

    tc.match.dungeon.rooms.access(GeneratorDoorwayDungeon2::ELEVATOR_ROOM_ID, [&](Room& room) {
        auto event = requireLastEvent(room);
        REQUIRE(event.action == EVENT_ELEVATOR);
    });
}

TEST_CASE("EventLog: TOGGLE", "[eventlog]") {
    TestController tc(GENERATOR_TUTORIAL);
    tc.isSkippingAnimations = true;
    tc.generate(0);
    REQUIRE(tc.match.start());

    // Move builder east into a room that has a toggler
    tc.moveCharacterToWall(Cardinal::east());
    REQUIRE(tc.isSuccess);

    const int currentRoomId = tc.latestPosition;

    // Find a toggler in the current room
    int togglerId = -1;
    for (Character& c : tc.match.dungeon.characters) {
        if ((c.role == ROLE_TOGGLER || c.role == ROLE_TOGGLER_BLUE || c.role == ROLE_TOGGLER_ORANGE)
                && c.location.roomId == currentRoomId) {
            togglerId = c.characterId;
            break;
        }
    }
    REQUIRE(togglerId != -1);

    tc.activateObjectCharacter(togglerId);
    REQUIRE(tc.isSuccess);

    tc.match.dungeon.rooms.access(currentRoomId, [&](Room& room) {
        auto event = requireLastEvent(room);
        REQUIRE(event.action == EVENT_TOGGLE);
    });
}

TEST_CASE("EventLog: LOOT_CHEST", "[eventlog]") {
    TestController tc(GENERATOR_TEST);
    tc.isSkippingAnimations = true;
    tc.generate(0);
    REQUIRE(tc.match.start());

    tc.giveItem(ITEM_KEY);

    // Find and open catalyst chest (key stays)
    int containerId = -1;
    for (const Chest& chest : tc.match.dungeon.chests) {
        if (chest.lock == LOCK_KEY_CATALYST_CLOSED) {
            containerId = chest.containerCharacterId;
            break;
        }
    }
    REQUIRE(containerId != -1);

    static ActivatorChestLockKey unlockActivator;
    tc.controller.activate(unlockActivator, Preactivation{
        .action = {
            .characterId = tc.builderOffset,
            .roomId = tc.latestPosition,
            .targetCharacterId = containerId,
        },
        .playerId = TestController::BUILDER_ID,
        .isSkippingAnimations = true,
    });
    REQUIRE(tc.codeset.getErrorTable() == Codeset::getEmptyTable());

    // Add a lootable key to the chest
    tc.match.dungeon.findChestByContainerId(containerId, tc.codeset.error).access([&](Chest& chest) {
        chest.inventory.giveItem(ITEM_KEY, tc.codeset.error);
    });
    REQUIRE(tc.codeset.getErrorTable() == Codeset::getEmptyTable());

    tc.lootInventory(containerId, ITEM_KEY);
    REQUIRE(tc.isSuccess);

    // The loot triggers a critter bite, so LOOT_CHEST_SOURCE may not be last — scan the log
    tc.match.dungeon.rooms.access(tc.latestPosition, [&](Room& room) {
        auto log = room.getEventLog();
        REQUIRE(log.size() > 0);
        bool found = false;
        for (int i = 0; i < (int)log.size(); i++) {
            if (log[i].action == EVENT_LOOT_CHEST) { found = true; break; }
        }
        REQUIRE(found);
    });
}

TEST_CASE("EventLog: CRITTER_BITE", "[eventlog]") {
    TestController tc(GENERATOR_TEST);
    tc.isSkippingAnimations = true;
    tc.generate(0);
    REQUIRE(tc.match.start());

    // Find a snake critter from a chest
    int critterId = -1;
    for (const Chest& chest : tc.match.dungeon.chests) {
        chest.inventory.accessItems(ITEM_CRITTER, [&](const Item& item) {
            critterId = item.stacks;
        });
        if (critterId != -1) break;
    }
    REQUIRE(critterId != -1);

    const int startRoomId = tc.latestPosition;
    tc.isSuccess = tc.controller.activate(Preactivation{
        .action = {
            .type = ACTION_CRITTER_BITE,
            .characterId = critterId,
            .roomId = startRoomId,
            .targetCharacterId = tc.builderOffset,
        },
        .playerId = TestController::BUILDER_ID,
        .isSkippingAnimations = true,
    });
    REQUIRE(tc.isSuccess);

    tc.match.dungeon.rooms.access(startRoomId, [&](Room& room) {
        auto event = requireLastEvent(room);
        REQUIRE(event.action == EVENT_CRITTER_BITE);
    });
}
