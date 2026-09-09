#include <catch2/catch_test_macros.hpp>
#include "ActionEnum.hpp"
#include "AnimationEnum.hpp"
#include "BehaviorEnum.hpp"
#include "Cardinal.hpp"
#include "Character.hpp"
#include "CodesetExpect.hpp"
#include "ConductEnum.hpp"
#include "ConductExpect.hpp"
#include "ConductMemory.hpp"
#include "ConductMemoryVariableEnum.hpp"
#include "GeneratorEnum.hpp"
#include "InventoryExpect.hpp"
#include "ItemEnum.hpp"
#include "Keyframe.hpp"
#include "LockEnum.hpp"
#include "Match.hpp"
#include "Preactivation.hpp"
#include "RoleEnum.hpp"
#include "TestController.hpp"

// Finds the most recently inserted WALKING_FROM_FLOOR_TO_FLOOR keyframe (highest t0),
// i.e. the one the monkey's *last* move actually wrote. Keyframe::insertKeyframe only
// evicts a slot once it has expired (see isAvailable()), so with MAX_KEYFRAMES slots a
// stale keyframe from an earlier hop can still be sitting in the array; scanning for
// "any" match would risk pairing a stale keyframe with the wrong hop's previousRoomId.
static Maybe<Keyframe> latestFloorToFloorKeyframe(const Array<Keyframe, Character::MAX_KEYFRAMES>& keyframes) {
    Maybe<Keyframe> latest;
    for (const auto& kf : keyframes) {
        if (kf.animation != ANIMATION_WALKING_FROM_FLOOR_TO_FLOOR) continue;
        if (latest.isEmpty() || kf.t0 > latest.orElse(Keyframe{}).t0) {
            latest = Maybe<Keyframe>(kf);
        }
    }
    return latest;
}

// A WALKING_FROM_FLOOR_TO_FLOOR keyframe whose source and destination cell are the
// same id is a no-op: AnimatedCharacter lerps cell N -> cell N on the frontend, so the
// walk animation plays but nothing visibly moves. See Keyframe::buildWalking's
// Location-overload (Keyframe.cpp) and its "floor -> floor, character passes 1 room"
// comment in Keyframe.hpp — this animation is only meant to represent motion within a
// single room, so a degenerate [N, N] means the two cell ids came from unrelated frames
// of reference (see hasMislabeledWallToFloorKeyframe below).
static bool hasDegenerateFloorToFloorKeyframe(const Array<Keyframe, Character::MAX_KEYFRAMES>& keyframes) {
    bool result = false;
    latestFloorToFloorKeyframe(keyframes).accessConst([&](const Keyframe& kf) {
        result = kf.data.begin()[0] == kf.data.begin()[1];
    });
    return result;
}

// TriggerEffectTraverseDoor (TriggerWrapper.cpp) moves an NPC across a room boundary by
// issuing a single ACTION_MOVE_TO_FLOOR straight into a free floor cell of the adjacent
// room, without ever placing the character in a LOCATION_DOOR position. Because
// ActivatorMoveToFloor's oldLocation is snapshotted from the character's location in the
// room it just left (MatchController::updateCharacterLocation), and that location is
// LOCATION_FLOOR, Keyframe::buildWalking's Location-overload sees FLOOR -> FLOOR on both
// ends and emits ANIMATION_WALKING_FROM_FLOOR_TO_FLOOR instead of
// ANIMATION_WALKING_FROM_WALL_TO_FLOOR — even though the character actually crossed a
// door into a different room.
//
// Detect this by comparing the latest FLOOR_TO_FLOOR keyframe's room0 against the room
// the character was in immediately before the move. room0 isn't a guess about where the
// monkey is headed: TriggerEffectTraverseDoor always issues ACTION_MOVE_TO_FLOOR with
// roomId = the destination room, and buildActivationContext resolves activation.room
// (and therefore Keyframe::room0) directly from that — true for both directions of
// travel (BEHAVIOR_STASH_TRAVERSING and BEHAVIOR_PICKPOCKET_RETURN_TO_START both route
// through the same handler). So a real same-room floor move has room0 == previousRoomId;
// a mislabeled cross-room hop never does.
//
// (In this map the monkey spawns in room 4 and its stash chest is in room 7 — see
// Generator.enum's MONKEY_PUZZLE_1 RemodelAuthorAllocateCharacter/ChestSeeder — so in
// practice previousRoomId only ever alternates between 4 and 7 here.)
static bool hasMislabeledWallToFloorKeyframe(const Array<Keyframe, Character::MAX_KEYFRAMES>& keyframes, int previousRoomId) {
    bool result = false;
    latestFloorToFloorKeyframe(keyframes).accessConst([&](const Keyframe& kf) {
        result = kf.room0 != previousRoomId;
    });
    return result;
}

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

    // This turn drives BEHAVIOR_STASH_TRAVERSING's TriggerEffectTraverseDoor, which
    // crosses a room boundary in a single ACTION_MOVE_TO_FLOOR call (see
    // hasMislabeledWallToFloorKeyframe above). That reproduces the frontend-reported
    // "monkey teleports at a door" bug: the server should emit
    // ANIMATION_WALKING_FROM_WALL_TO_FLOOR for a door crossing, and never a
    // FLOOR_TO_FLOOR walk whose data is degenerate ([N, N], zero displacement on the
    // client). Both checks are expected to hold; either failing reproduces the bug
    // reported from the frontend animation log.
    const int monkeyRoomIdBeforeStashTraversal = getMonkeyRoomId();
    tc.endTurn();
    REQUIRE_THAT(codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true)));

    monkeyConductPtr.access([&](Conduct& conduct) {
        REQUIRE_THAT(conduct, MatchesConductExpect(
            ConductExpect{}
                .expectState(CONDUCT_PICKPOCKET, BEHAVIOR_PICKPOCKET_RETURN_TO_START)
        ));
    });
    monkeyPtr.accessConst([&](const Character& monkey) {
        REQUIRE_FALSE(hasDegenerateFloorToFloorKeyframe(monkey.keyframes));
        REQUIRE_FALSE(hasMislabeledWallToFloorKeyframe(monkey.keyframes, monkeyRoomIdBeforeStashTraversal));
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
