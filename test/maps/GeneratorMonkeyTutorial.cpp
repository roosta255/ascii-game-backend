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
#include "TestController.hpp"

TEST_CASE("Monkey steals ITEM_COIN from builder in shared room", "[match][GENERATOR_MONKEY_TUTORIAL][monkey]") {
    TestController tc(GENERATOR_MONKEY_TUTORIAL);
    tc.isSkippingAnimations = true;

    tc.generate(0);
    REQUIRE(tc.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(tc.match.start());

    // Seed builder with a coin so the monkey has something to steal.
    tc.giveItem(ITEM_COIN);

    // Exhaust the builder's action budget so TRAIT_ACTION_READY is false,
    // which is the condition that makes the builder pickpocketable.
    tc.builderCharacterPtr->actions = 2;
    tc.controller.updateTraits(*tc.builderCharacterPtr);

    // Move builder north into the monkey's starting room {1,2,0,0}.
    //
    // During controller.activate:
    //   PICKPOCKET_SEARCHING observer fires → ATTEMPT.
    //   ATTEMPT proposer fires inline → ACTION_PICKPOCKET: coin transfers to monkey.
    //   ON_PICKPOCKET_AS_ACTOR fires inline → STASH_FINDING.
    //   STASH_FINDING proposer fires inline → scan chests → STASH_PATHING.
    //
    // During the tickNpcConducts that TestController::updateEverything fires after
    // every successful action:
    //   STASH_PATHING proposer fires → BFS flood-fill → writes door-direction bits
    //   → STASH_TRAVERSING.
    tc.moveCharacterToWall(Cardinal::north());
    REQUIRE(tc.isSuccess);
    tc.moveCharacterToFloor(4);
    REQUIRE(tc.isSuccess);
    const int monkeyRoomId = tc.latestPosition;

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
    REQUIRE(monkeyLocRoomId == monkeyRoomId);

    // Verify STASH_TRAVERSING + flood-fill path toward the stash chest (room 2).
    //
    // Room numbering on z=0 floor (width=3): roomId = x + y*3
    //   (0,2)=6  (1,2)=7  (2,2)=8    ← monkey starts at 7
    //   (0,1)=3  (1,1)=4  (2,1)=5
    //   (0,0)=0  (1,0)=1  (2,0)=2    ← stash chest at 2
    //
    // SEARCH_TARGETS is a room-bitmask: chest is at roomId 2, so bit 2 → value 4.
    //
    // Wall layout forces one snake path 7→4→3→0→1→2.  The flood fill starts at
    // room 2 and expands outward; againstBits records, for each dest room, the
    // direction to take FROM that room to head back toward the origin:
    //   BFS step 2→W→1: againstBits[E] |= (1<<1)
    //   BFS step 1→W→0: againstBits[E] |= (1<<0)   → EAST  = 3
    //   BFS step 0→N→3: againstBits[S] |= (1<<3)
    //   BFS step 3→N→6: againstBits[S] |= (1<<6)
    //   BFS step 3→E→4: againstBits[W] |= (1<<4)
    //   BFS step 4→N→7: againstBits[S] |= (1<<7)   → SOUTH = 8+64+128+256 = 456
    //   BFS step 4→E→5: againstBits[W] |= (1<<5)   → WEST  = 16+32 = 48
    //   BFS step 5→N→8: againstBits[S] |= (1<<8)
    //                                                  NORTH = 0
    tc.controller.getConductByCharacterId(monkeyCharId).access([&](Conduct& conduct) {
        REQUIRE_THAT(conduct, MatchesConductExpect(
            ConductExpect{}
                .expectState(CONDUCT_PICKPOCKET, BEHAVIOR_STASH_TRAVERSING)
                .expectVar(CONDUCT_MEMORY_SEARCH_TARGETS,       4)
                .expectVar(CONDUCT_MEMORY_PATHFIND_DOOR_NORTH,  0)
                .expectVar(CONDUCT_MEMORY_PATHFIND_DOOR_EAST,   3)
                .expectVar(CONDUCT_MEMORY_PATHFIND_DOOR_SOUTH, 456)
                .expectVar(CONDUCT_MEMORY_PATHFIND_DOOR_WEST,   48)
        ));
    });

    // endTurn → tickNpcConducts.
    //   STASH_TRAVERSING proposer fires ACTION_MOVE_TO_FLOOR into the adjacent room.
    //   Each move's ON_MOVE_AS_ACTOR inline handler re-fires buildAndExecuteProposals,
    //   chaining the full path 7→4→3→0→1→2.
    //   On arriving at room 2 the chest scan fires → STASH_DEPOSITING.
    //   STASH_DEPOSITING proposer fires ACTION_DEPOSIT inline → ON_DEPOSIT_AS_ACTOR
    //   → PICKPOCKET_SEARCHING.  All within this single tick.
    tc.endTurn();
    REQUIRE(tc.isSuccess);

    // Monkey has deposited the coin and reset to searching.
    tc.controller.getConductByCharacterId(monkeyCharId).access([&](Conduct& conduct) {
        REQUIRE_THAT(conduct, MatchesConductExpect(
            ConductExpect{}.expectState(CONDUCT_PICKPOCKET, BEHAVIOR_PICKPOCKET_SEARCHING)
        ));
    });

    // Builder's inventory no longer contains the coin.
    int coinAfter = 0;
    tc.playerPtr->getInventory(tc.match.dungeon).accessItem(ITEM_COIN, [&](const auto& item) { coinAfter = item.stacks; });
    REQUIRE(coinAfter == 0);
}
