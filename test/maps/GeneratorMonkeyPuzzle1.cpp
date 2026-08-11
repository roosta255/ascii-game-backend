#include <catch2/catch_test_macros.hpp>
#include "ActionEnum.hpp"
#include "BehaviorEnum.hpp"
#include "Cardinal.hpp"
#include "CodesetExpect.hpp"
#include "ConductEnum.hpp"
#include "ConductExpect.hpp"
#include "ConductMemory.hpp"
#include "ConductMemoryVariableEnum.hpp"
#include "GeneratorEnum.hpp"
#include "InventoryExpect.hpp"
#include "ItemEnum.hpp"
#include "LockEnum.hpp"
#include "Match.hpp"
#include "Preactivation.hpp"
#include "RoleEnum.hpp"
#include "TestController.hpp"

TEST_CASE("Monkey steals ITEM_COIN from builder in shared room", "[match][GENERATOR_MONKEY_PUZZLE_1]") {
    TestController tc(GENERATOR_MONKEY_PUZZLE_1_TEST);
    Codeset& codeset = tc.codeset;
    tc.isSkippingAnimations = true;

    tc.generate(0);
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    REQUIRE(tc.match.start());

    // Locate the monkey NPC.
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
    REQUIRE(monkeyLocRoomId == 4);

    auto monkeyConductPtr = tc.controller.getConductByCharacterId(monkeyCharId);

    monkeyConductPtr.access([&](Conduct& conduct) {
        REQUIRE_THAT(conduct, MatchesConductExpect(
            ConductExpect{}
                .expectState(CONDUCT_PICKPOCKET, BEHAVIOR_PICKPOCKET_INIT)
        ));
    });

    auto monkeyPtr = tc.controller.match.getCharacter(monkeyCharId, tc.codeset.error);
    Inventory monkeyInventory = monkeyPtr.map<Inventory>([&](Character& monkey){
        return monkey.getInventory(tc.controller.match.dungeon);}).orElse(Inventory());
    const auto getMonkeyRoomId = [&](){
        return monkeyPtr.mapConst<int>([&](const Character& monkey){
            return monkey.location.roomId;
        }).orElse(-1);
    };
    REQUIRE(getMonkeyRoomId() == 4);

    // verify monkey return variables were setup
    tc.endTurn();
    monkeyConductPtr.access([&](Conduct& conduct) {
        REQUIRE_THAT(conduct, MatchesConductExpect(
            ConductExpect{}
                .expectState(CONDUCT_PICKPOCKET, BEHAVIOR_PICKPOCKET_SEARCHING)
                .expectVar(CONDUCT_MEMORY_ROOM_ID, 4)
        ));
    });

    // These next actions get the key
    // activate the toggler
    tc.activateObjectCharacter(ROLE_TOGGLER_BLUE); // in room 3
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    tc.moveCharacterToWall(Cardinal::east()); // to room 4
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.endTurn();
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.moveCharacterToWall(Cardinal::east()); // to room 5
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.lootInventory(ROLE_CHEST, ITEM_KEY);
    REQUIRE_THAT(tc.playerPtr->getInventory(tc.match.dungeon), MatchesInventoryExpect(
        InventoryExpect{}
            .expectStacks(ITEM_COIN, 0)
            .expectStacks(ITEM_KEY, 1)
    ));
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.moveCharacterToFloor(5); // turn around into door
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.endTurn();
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.moveCharacterToWall(Cardinal::west()); // to room 4
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.endTurn();
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.moveCharacterToFloor(5); // turn around into door
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.endTurn();
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    monkeyConductPtr.access([&](Conduct& conduct) {
        REQUIRE_THAT(conduct, MatchesConductExpect(
            ConductExpect{}
                .expectState(CONDUCT_PICKPOCKET, BEHAVIOR_STASH_TRAVERSING)
        ));
    });
    REQUIRE_THAT(tc.playerPtr->getInventory(tc.match.dungeon), MatchesInventoryExpect(
        InventoryExpect{}.expectStacks(ITEM_KEY, 0)
    ));

    tc.endTurn();
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true)));

    monkeyConductPtr.access([&](Conduct& conduct) {
        REQUIRE_THAT(conduct, MatchesConductExpect(
            ConductExpect{}
                .expectState(CONDUCT_PICKPOCKET, BEHAVIOR_PICKPOCKET_RETURN_TO_START)
        ));
    });

    // get back the key
    tc.moveCharacterToWall(Cardinal::north()); // to room 7
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.endTurn();
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.lootInventory(ROLE_CADDY, ITEM_KEY);
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    REQUIRE_THAT(tc.playerPtr->getInventory(tc.match.dungeon), MatchesInventoryExpect(
        InventoryExpect{}
            .expectStacks(ITEM_COIN, 0)
            .expectStacks(ITEM_KEY, 1)
    ));

    // stash the key into sharer
    tc.moveCharacterToWall(Cardinal::east()); // to room 8
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.activateLock(Cardinal::south()); // sets key down
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    REQUIRE_THAT(tc.playerPtr->getInventory(tc.match.dungeon), MatchesInventoryExpect(InventoryExpect{}.expectStacks(ITEM_KEY, 0)));
    tc.endTurn();
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    // toggle the orange doors open without losing the key
    tc.moveCharacterToFloor(5); // turn around into door
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.moveCharacterToWall(Cardinal::west()); // to room 7
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.endTurn();
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.moveCharacterToWall(Cardinal::south()); // to room 4
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.endTurn();
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.moveCharacterToWall(Cardinal::west()); // to room 3
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.moveCharacterToFloor(5); // turn around into door
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.endTurn();
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.activateObjectCharacter(ROLE_TOGGLER_ORANGE); // in room 3
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    // get the key to unlock the keeper and loopback latch
    tc.moveCharacterToWall(Cardinal::east()); // to room 4
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.endTurn();
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.moveCharacterToWall(Cardinal::east()); // to room 5
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.activateLock(Cardinal::north()); // picks key up
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.endTurn();
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.moveCharacterToWall(Cardinal::south()); // to room 2
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.activateLock(Cardinal::west()); // sets key down to unlock keeper
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.endTurn();
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.moveCharacterToWall(Cardinal::west()); // to room 1
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.activateLock(Cardinal::north()); // unlocks loopback latch
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    // loopback through monkeys to unlock toggler to then get second key
    tc.moveCharacterToWall(Cardinal::north()); // to room 4
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.endTurn();
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.moveCharacterToWall(Cardinal::west()); // to room 3
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.endTurn();
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.activateObjectCharacter(ROLE_TOGGLER_BLUE); // in room 3
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.moveCharacterToFloor(5); // turn around into door
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.endTurn();
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.moveCharacterToWall(Cardinal::east()); // to room 4
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.endTurn();
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    // finish taking room 1 key
    tc.moveCharacterToWall(Cardinal::south()); // to room 1
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.lootInventory(ROLE_CHEST, ITEM_KEY);
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.activateLock(Cardinal::west()); // insert key into west door
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    // reloop around monkeys to unlock chest in room 0
    tc.moveCharacterToFloor(5); // clear doorway
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.endTurn();
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.moveCharacterToWall(Cardinal::north()); // to room 4
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    tc.moveCharacterToWall(Cardinal::west()); // to room 3
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.endTurn();
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    tc.activateObjectCharacter(ROLE_TOGGLER_ORANGE);
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    tc.moveCharacterToFloor(5); // clear doorway
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    tc.moveCharacterToWall(Cardinal::east()); // to room 4
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.endTurn();
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    tc.moveCharacterToWall(Cardinal::south()); // to room 1
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    tc.moveCharacterToWall(Cardinal::west()); // to room 0
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    tc.lootInventory(ROLE_CHEST, ITEM_KEY);
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.endTurn();
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    tc.moveCharacterToFloor(5); // clear doorway
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    // all keys have been freed from chests, gather 3 keys
    tc.moveCharacterToWall(Cardinal::east()); // to room 1
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.endTurn();
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    tc.moveCharacterToFloor(5); // clear doorway
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    tc.activateLock(Cardinal::west()); // take key from west door
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    tc.moveCharacterToWall(Cardinal::east()); // to room 2
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.endTurn();
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    tc.moveCharacterToFloor(5); // clear doorway
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    tc.activateLock(Cardinal::west()); // take key from west door
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.endTurn();
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    tc.moveCharacterToWall(Cardinal::north()); // to room 5
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    REQUIRE_THAT(tc.playerPtr->getInventory(tc.match.dungeon),
        MatchesInventoryExpect(
            InventoryExpect{}.expectStacks(ITEM_KEY, 3)
        ));

    // 3 keys are gathered, make a failing run past 2 monkeys
    REQUIRE(getMonkeyRoomId() == 4);
    tc.moveCharacterToWall(Cardinal::west()); // to room 4
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.endTurn();
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    tc.moveCharacterToFloor(6);
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.endTurn();
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true)));

    tc.moveCharacterToFloor(7);
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.endTurn();
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true)));

    tc.moveCharacterToFloor(8);
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.endTurn();
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    tc.moveCharacterToFloor(7);
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.endTurn();
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    tc.moveCharacterToFloor(8);
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.endTurn();
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true)));

    tc.moveCharacterToFloor(7);
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.endTurn();
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    tc.moveCharacterToFloor(8);
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.endTurn();
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    REQUIRE_THAT(tc.playerPtr->getInventory(tc.match.dungeon),
        MatchesInventoryExpect(
            InventoryExpect{}.expectStacks(ITEM_KEY, 0)
        ));

    return;

    // regain 3 keys after monkey thefts
    tc.moveCharacterToWall(Cardinal::north()); // to room 7
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    tc.lootInventory(ROLE_CHEST, ITEM_KEY);
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.lootInventory(ROLE_CHEST, ITEM_KEY);
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.lootInventory(ROLE_CHEST, ITEM_KEY);
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    REQUIRE_THAT(tc.playerPtr->getInventory(tc.match.dungeon),
        MatchesInventoryExpect(
            InventoryExpect{}.expectStacks(ITEM_KEY, 3)
        ));

    // make a successful run past 2 monkeys
    tc.moveCharacterToFloor(5); // clear doorway
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    tc.moveCharacterToWall(Cardinal::south()); // to room 4
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    tc.moveCharacterToWall(Cardinal::west()); // to room 3
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    // use key on last keeper door
    tc.activateLock(Cardinal::north());
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

    tc.moveCharacterToWall(Cardinal::north()); // to room 6
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
}
