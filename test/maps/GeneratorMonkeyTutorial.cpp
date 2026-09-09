#include <catch2/catch_test_macros.hpp>
#include "ActionEnum.hpp"
#include "AnimationEnum.hpp"
#include "BehaviorEnum.hpp"
#include "Cardinal.hpp"
#include "Character.hpp"
#include "ConductEnum.hpp"
#include "ConductExpect.hpp"
#include "ConductMemory.hpp"
#include "ConductMemoryVariableEnum.hpp"
#include "GeneratorEnum.hpp"
#include "Inventory.hpp"
#include "InventoryExpect.hpp"
#include "ItemEnum.hpp"
#include "Keyframe.hpp"
#include "LockEnum.hpp"
#include "Match.hpp"
#include "Preactivation.hpp"
#include "RoleEnum.hpp"
#include "CodesetExpect.hpp"
#include "TestController.hpp"

// Matches any of the walking-family animations (the exact variant depends on
// whether the monkey is transitioning between doors and/or floors).
static bool hasWalkingKeyframe(const Array<Keyframe, Character::MAX_KEYFRAMES>& keyframes) {
    for (const auto& kf : keyframes) {
        switch (kf.animation) {
            case ANIMATION_WALKING_FROM_WALL_TO_WALL:
            case ANIMATION_WALKING_FROM_WALL_TO_FLOOR:
            case ANIMATION_WALKING_FROM_FLOOR_TO_WALL:
            case ANIMATION_WALKING_FROM_FLOOR_TO_FLOOR:
                return true;
            default:
                break;
        }
    }
    return false;
}

// Matches any of the bounce-family animations. ACTION_PICKPOCKET's onSuccess
// effect queries ANIMATION_BOUNCE_FROM_FLOOR_TO_LOCK as a seed (see Action.enum),
// but ActivationAddTargetKeyframe/buildTargetKeyframe re-resolves it via
// AnimationFlyweight::queryAnimation against the subject/target's actual
// FLOOR/DOOR semantics — with both monkey and victim on the floor, it resolves
// to ANIMATION_BOUNCE_FROM_FLOOR_TO_FLOOR, not the literal seed value.
static bool hasBounceKeyframe(const Array<Keyframe, Character::MAX_KEYFRAMES>& keyframes) {
    for (const auto& kf : keyframes) {
        switch (kf.animation) {
            case ANIMATION_BOUNCE_FROM_DOOR_TO_LOCK:
            case ANIMATION_BOUNCE_FROM_DOOR_TO_FLOOR:
            case ANIMATION_BOUNCE_FROM_FLOOR_TO_LOCK:
            case ANIMATION_BOUNCE_FROM_FLOOR_TO_FLOOR:
                return true;
            default:
                break;
        }
    }
    return false;
}

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
static bool hasMislabeledWallToFloorKeyframe(const Array<Keyframe, Character::MAX_KEYFRAMES>& keyframes, int previousRoomId) {
    bool result = false;
    latestFloorToFloorKeyframe(keyframes).accessConst([&](const Keyframe& kf) {
        result = kf.room0 != previousRoomId;
    });
    return result;
}

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
    REQUIRE(monkeyLocRoomId == 7);

    tc.controller.getConductByCharacterId(monkeyCharId).access([&](Conduct& conduct) {
        REQUIRE_THAT(conduct, MatchesConductExpect(
            ConductExpect{}
                .expectState(CONDUCT_PICKPOCKET, BEHAVIOR_PICKPOCKET_INIT)
        ));
    });

    auto monkeyPtr = tc.controller.match.getCharacter(monkeyCharId, tc.codeset.error);
    Inventory monkeyInventory = monkeyPtr.map<Inventory>([&](Character& monkey){
        return monkey.getInventory(tc.controller.match.dungeon);}).orElse(Inventory());
    auto accessMonkeyInventory = [&](std::function<void(Inventory)> consumer){
        return monkeyPtr.access([&](Character& monkey){
            consumer(monkey.getInventory(tc.controller.match.dungeon));
        });
    };
    const auto getMonkeyRoomId = [&](){
        return monkeyPtr.mapConst<int>([&](const Character& monkey){
            return monkey.location.roomId;
        }).orElse(-1);
    };
    REQUIRE(getMonkeyRoomId() == 7);
    
    // verify monkey return variables were setup
    tc.endTurn();
    tc.controller.getConductByCharacterId(monkeyCharId).access([&](Conduct& conduct) {
        REQUIRE_THAT(conduct, MatchesConductExpect(
            ConductExpect{}
                .expectState(CONDUCT_PICKPOCKET, BEHAVIOR_PICKPOCKET_SEARCHING)
                .expectVar(CONDUCT_MEMORY_ROOM_ID,           7)
                .expectVar(CONDUCT_MEMORY_RETURN_DOOR_NORTH, 17)
                .expectVar(CONDUCT_MEMORY_RETURN_DOOR_EAST,  8)
                .expectVar(CONDUCT_MEMORY_RETURN_DOOR_SOUTH, 320)
                .expectVar(CONDUCT_MEMORY_RETURN_DOOR_WEST,  38)
        ));
    });
    REQUIRE_THAT(monkeyInventory, MatchesInventoryExpect(InventoryExpect{}.expectStacks(ITEM_COIN, 0).expectStacks(ITEM_KEY, 0)));

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
    REQUIRE_THAT(tc.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.moveCharacterToFloor(4);
    REQUIRE_THAT(tc.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));
    tc.endTurn();
    REQUIRE_THAT(tc.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true).expectNoErrors()));

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
    tc.endTurn();
    REQUIRE_THAT(monkeyInventory, MatchesInventoryExpect(InventoryExpect{}.expectStacks(ITEM_COIN, 1).expectStacks(ITEM_KEY, 0)));

    // The successful ACTION_PICKPOCKET writes a bounce keyframe onto the acting
    // monkey (ANIMATION_ACT_SUBJECT_TO_TARGET targets the subject, i.e. the
    // monkey itself, not the victim) — see Action.enum's PICKPOCKET onSuccess.
    monkeyPtr.accessConst([&](const Character& monkey) {
        REQUIRE(hasBounceKeyframe(monkey.keyframes));
    });

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
    REQUIRE_THAT(tc.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true)));
    REQUIRE(getMonkeyRoomId() == 4);
    
    // Verify Monkey moving through rooms to chest.
    //
    // Each of these turns advances the monkey one room via
    // BEHAVIOR_STASH_TRAVERSING's TriggerEffectTraverseDoor, which crosses a room
    // boundary in a single ACTION_MOVE_TO_FLOOR call (see hasMislabeledWallToFloorKeyframe
    // above). That reproduces the frontend-reported "monkey teleports at a door" bug: the
    // server should emit ANIMATION_WALKING_FROM_WALL_TO_FLOOR for a door crossing, and
    // never a FLOOR_TO_FLOOR walk whose data is degenerate ([N, N], zero displacement on
    // the client). Both checks are expected to hold; either failing reproduces the bug
    // reported from the frontend animation log.
    int previousMonkeyRoomId = getMonkeyRoomId();
    tc.endTurn();
    REQUIRE_THAT(monkeyInventory, MatchesInventoryExpect(InventoryExpect{}.expectStacks(ITEM_COIN, 1).expectStacks(ITEM_KEY, 0)));
    REQUIRE_THAT(tc.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true)));
    tc.controller.getConductByCharacterId(monkeyCharId).access([&](Conduct& conduct) {
        REQUIRE_THAT(conduct, MatchesConductExpect(
            ConductExpect{}.expectState(CONDUCT_PICKPOCKET, BEHAVIOR_STASH_TRAVERSING)
        ));
    });
    REQUIRE(getMonkeyRoomId() == 3);

    // The monkey's own move (room 4 -> 3) should have written a walking
    // keyframe onto itself instead of leaving the array at ANIMATION_NIL.
    monkeyPtr.accessConst([&](const Character& monkey) {
        REQUIRE(hasWalkingKeyframe(monkey.keyframes));
        REQUIRE_FALSE(hasDegenerateFloorToFloorKeyframe(monkey.keyframes));
        REQUIRE_FALSE(hasMislabeledWallToFloorKeyframe(monkey.keyframes, previousMonkeyRoomId));
    });

    previousMonkeyRoomId = getMonkeyRoomId();
    tc.endTurn();
    REQUIRE_THAT(tc.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true)));
    REQUIRE(getMonkeyRoomId() == 0);
    monkeyPtr.accessConst([&](const Character& monkey) {
        REQUIRE_FALSE(hasDegenerateFloorToFloorKeyframe(monkey.keyframes));
        REQUIRE_FALSE(hasMislabeledWallToFloorKeyframe(monkey.keyframes, previousMonkeyRoomId));
    });

    previousMonkeyRoomId = getMonkeyRoomId();
    tc.endTurn();
    REQUIRE_THAT(tc.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true)));
    REQUIRE(getMonkeyRoomId() == 1);
    monkeyPtr.accessConst([&](const Character& monkey) {
        REQUIRE_FALSE(hasDegenerateFloorToFloorKeyframe(monkey.keyframes));
        REQUIRE_FALSE(hasMislabeledWallToFloorKeyframe(monkey.keyframes, previousMonkeyRoomId));
    });

    previousMonkeyRoomId = getMonkeyRoomId();
    tc.endTurn();
    REQUIRE_THAT(tc.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true)));
    REQUIRE(getMonkeyRoomId() == 2);
    monkeyPtr.accessConst([&](const Character& monkey) {
        REQUIRE_FALSE(hasDegenerateFloorToFloorKeyframe(monkey.keyframes));
        REQUIRE_FALSE(hasMislabeledWallToFloorKeyframe(monkey.keyframes, previousMonkeyRoomId));
    });

    // Verify that monkey has returned to spawn point

    // Monkey has deposited the coin and is returning to start.
    tc.controller.getConductByCharacterId(monkeyCharId).access([&](Conduct& conduct) {
        REQUIRE_THAT(conduct, MatchesConductExpect(
            ConductExpect{}.expectState(CONDUCT_PICKPOCKET, BEHAVIOR_PICKPOCKET_RETURN_TO_START)
        ));
    });

    // Builder's inventory no longer contains the coin.
    int coinAfter = 0;
    tc.playerPtr->getInventory(tc.match.dungeon).accessItem(ITEM_COIN, [&](const auto& item) { coinAfter = item.stacks; });
    REQUIRE(coinAfter == 0);

    // verify monkey is returning to start
    previousMonkeyRoomId = getMonkeyRoomId();
    tc.endTurn();
    REQUIRE_THAT(tc.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true)));
    REQUIRE(getMonkeyRoomId() == 1);
    monkeyPtr.accessConst([&](const Character& monkey) {
        REQUIRE_FALSE(hasDegenerateFloorToFloorKeyframe(monkey.keyframes));
        REQUIRE_FALSE(hasMislabeledWallToFloorKeyframe(monkey.keyframes, previousMonkeyRoomId));
    });

    previousMonkeyRoomId = getMonkeyRoomId();
    tc.endTurn();
    REQUIRE_THAT(tc.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true)));
    REQUIRE(getMonkeyRoomId() == 0);
    monkeyPtr.accessConst([&](const Character& monkey) {
        REQUIRE_FALSE(hasDegenerateFloorToFloorKeyframe(monkey.keyframes));
        REQUIRE_FALSE(hasMislabeledWallToFloorKeyframe(monkey.keyframes, previousMonkeyRoomId));
    });

    previousMonkeyRoomId = getMonkeyRoomId();
    tc.endTurn();
    REQUIRE_THAT(tc.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true)));
    REQUIRE(getMonkeyRoomId() == 3);
    monkeyPtr.accessConst([&](const Character& monkey) {
        REQUIRE_FALSE(hasDegenerateFloorToFloorKeyframe(monkey.keyframes));
        REQUIRE_FALSE(hasMislabeledWallToFloorKeyframe(monkey.keyframes, previousMonkeyRoomId));
    });

    previousMonkeyRoomId = getMonkeyRoomId();
    tc.endTurn();
    REQUIRE_THAT(tc.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true)));
    REQUIRE(getMonkeyRoomId() == 4);
    monkeyPtr.accessConst([&](const Character& monkey) {
        REQUIRE_FALSE(hasDegenerateFloorToFloorKeyframe(monkey.keyframes));
        REQUIRE_FALSE(hasMislabeledWallToFloorKeyframe(monkey.keyframes, previousMonkeyRoomId));
    });

    previousMonkeyRoomId = getMonkeyRoomId();
    tc.endTurn();
    REQUIRE_THAT(tc.codeset, MatchesCodesetExpect(CodesetExpect{}.expectIsLatestSuccessFlag(true)));
    REQUIRE(getMonkeyRoomId() == 7);
    monkeyPtr.accessConst([&](const Character& monkey) {
        REQUIRE_FALSE(hasDegenerateFloorToFloorKeyframe(monkey.keyframes));
        REQUIRE_FALSE(hasMislabeledWallToFloorKeyframe(monkey.keyframes, previousMonkeyRoomId));
    });

    // verify monkey is searching again
    tc.controller.getConductByCharacterId(monkeyCharId).access([&](Conduct& conduct) {
        REQUIRE_THAT(conduct, MatchesConductExpect(
            ConductExpect{}.expectState(CONDUCT_PICKPOCKET, BEHAVIOR_PICKPOCKET_SEARCHING)
        ));
    });
}
