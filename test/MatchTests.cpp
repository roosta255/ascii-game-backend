#include "ActionFlyweight.hpp"
#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include "Cardinal.hpp"
#include "Character.hpp"
#include "CodeEnum.hpp"
#include "Codeset.hpp"
#include "Dungeon.hpp"
#include "GeneratorEnum.hpp"
#include "GeneratorTutorial.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Preactivation.hpp"
#include "Room.hpp"
#include "RoleEnum.hpp"
#include "TestController.hpp"

TEST_CASE("Match instantiation", "[match]") {
    Match match;

    REQUIRE(true);
}

TEST_CASE("Match generation", "[match]") {
    TestController controller(GENERATOR_TUTORIAL);
    
    // Generate test layout
    controller.controller.generate(0);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
/*
    // Start the match
    REQUIRE(controller.match.start());

    REQUIRE(controller.inventory.isEmpty);
    REQUIRE(controller.inventory.keys == 0);

    REQUIRE(controller.match.titan.player.account == "");
    REQUIRE(controller.match.builders.access(0, [&](Builder& builder) {
        REQUIRE(builder.player.account == controller.match.host);
    }));
    */
}

TEST_CASE("Test Character Animation", "[walking]") {
    CodeEnum error = CODE_UNKNOWN_ERROR;

    // Setup
    Match match;
    Codeset codeset;
    MatchController controller(match, codeset);
    CodeEnum result = CODE_SUCCESS;
    const std::string builderId = "builder_1";
    const std::string hostId = builderId;

    match.host = hostId;
    match.filename = "match_002";
    match.generator = GENERATOR_TUTORIAL;

    // Generate tutorial layout
    bool isSuccess = controller.generate(0);
    REQUIRE(codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(isSuccess);

    // Start the match
    REQUIRE(match.start());

    // Get the actual character offset of the first builder
    int builderOffset;
    const bool isBuilder0Accessed = match.builders.access(0, [&](Builder& builder) {
        REQUIRE(builder.character.keyframes.access(0, [&](const Keyframe& keyframe){
            REQUIRE(keyframe.animation == ANIMATION_NIL);
            REQUIRE(keyframe.t0 == Timestamp::nil());
            REQUIRE(keyframe.t1 == Timestamp::nil());
            REQUIRE(keyframe.isAvailable());
        }));

        for (const auto& keyframe: builder.character.keyframes) {
            REQUIRE(keyframe.animation == ANIMATION_NIL);
            REQUIRE(keyframe.t0 == Timestamp::nil());
            REQUIRE(keyframe.t1 == Timestamp::nil());
            REQUIRE(keyframe.isAvailable() == true);
        }

        REQUIRE(match.containsCharacter(builder.character, builderOffset));
        const auto t0 = Timestamp::buildTimestamp(2200);
        const auto t1 = t0 + MatchController::MOVE_ANIMATION_DURATION;

        REQUIRE(builder.character.location == Location::makeFloor(0,CHANNEL_CORPOREAL,0));

        // walk the character to wall
        Preactivation preactivation{
            .action = {
                .type = ACTION_MOVE_TO_DOOR,
                .characterId = builderOffset,
                .roomId = 0,
                .direction = Cardinal::east(),
            },
            .playerId = builderId,
            .isSkippingAnimations = false,
            .time = t0
        };

        isSuccess = controller.activate(preactivation);
        REQUIRE(codeset.getErrorTable() == Codeset::getEmptyTable());
        REQUIRE(isSuccess);

        const auto it = std::find_if(builder.character.keyframes.begin(), builder.character.keyframes.end(),
            [](const Keyframe& keyframe){return keyframe.animation == ANIMATION_WALKING_FROM_FLOOR_TO_WALL;});

        REQUIRE(it != builder.character.keyframes.end());
        REQUIRE(it->t0 == t0);
        REQUIRE(it->t1 == t1);
    });

    REQUIRE(isBuilder0Accessed);
}

TEST_CASE("Test Character Animation - movement keyframes queue sequentially", "[walking]") {
    Match match;
    Codeset codeset;
    MatchController controller(match, codeset);
    const std::string builderId = "builder_1";

    match.host = builderId;
    match.filename = "match_anim_queue";
    match.generator = GENERATOR_TUTORIAL;

    REQUIRE(controller.generate(0));
    REQUIRE(codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(match.start());

    match.builders.access(0, [&](Builder& builder) {
        int builderOffset;
        REQUIRE(match.containsCharacter(builder.character, builderOffset));

        // First move at t=1000
        const auto t0 = Timestamp::buildTimestamp(1000);
        const auto expectedT1 = t0 + MatchController::MOVE_ANIMATION_DURATION;

        Preactivation firstMove{
            .action = {
                .type = ACTION_MOVE_TO_DOOR,
                .characterId = builderOffset,
                .roomId = 0,
                .direction = Cardinal::east(),
            },
            .playerId = builderId,
            .isSkippingAnimations = false,
            .time = t0
        };
        REQUIRE(controller.activate(firstMove));
        REQUIRE(codeset.getErrorTable() == Codeset::getEmptyTable());

        // Second move at t=1000 (same time, animation still in-flight)
        // Expected: new keyframe starts at expectedT1 (end of first), not at t0
        const auto secondExpectedT0 = expectedT1;
        const auto secondExpectedT1 = secondExpectedT0 + MatchController::MOVE_ANIMATION_DURATION;

        Preactivation secondMove{
            .action = {
                .type = ACTION_MOVE_TO_FLOOR,
                .characterId = builderOffset,
                .roomId = 0,
                .floorId = 0,
            },
            .playerId = builderId,
            .isSkippingAnimations = false,
            .time = t0
        };
        REQUIRE(controller.activate(secondMove));
        REQUIRE(codeset.getErrorTable() == Codeset::getEmptyTable());

        // Find the second (floor) walking keyframe
        const auto it = std::find_if(builder.character.keyframes.begin(), builder.character.keyframes.end(),
            [](const Keyframe& kf){ return kf.animation == ANIMATION_WALKING_FROM_WALL_TO_FLOOR; });

        REQUIRE(it != builder.character.keyframes.end());
        REQUIRE(it->t0 == secondExpectedT0);
        REQUIRE(it->t1 == secondExpectedT1);
    });
}

TEST_CASE("Test Character Animation - movement keyframe does not delay when previous expired", "[walking]") {
    Match match;
    Codeset codeset;
    MatchController controller(match, codeset);
    const std::string builderId = "builder_1";

    match.host = builderId;
    match.filename = "match_anim_noqueue";
    match.generator = GENERATOR_TUTORIAL;

    REQUIRE(controller.generate(0));
    REQUIRE(codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(match.start());

    match.builders.access(0, [&](Builder& builder) {
        int builderOffset;
        REQUIRE(match.containsCharacter(builder.character, builderOffset));

        // First move at t=1000
        const auto t0 = Timestamp::buildTimestamp(1000);

        Preactivation firstMove{
            .action = {
                .type = ACTION_MOVE_TO_DOOR,
                .characterId = builderOffset,
                .roomId = 0,
                .direction = Cardinal::east(),
            },
            .playerId = builderId,
            .isSkippingAnimations = false,
            .time = t0
        };
        REQUIRE(controller.activate(firstMove));
        REQUIRE(codeset.getErrorTable() == Codeset::getEmptyTable());

        // Second move at a time well past the first animation's t1
        const auto laterTime = t0 + MatchController::MOVE_ANIMATION_DURATION * 10;

        Preactivation secondMove{
            .action = {
                .type = ACTION_MOVE_TO_FLOOR,
                .characterId = builderOffset,
                .roomId = 0,
                .floorId = 0,
            },
            .playerId = builderId,
            .isSkippingAnimations = false,
            .time = laterTime
        };
        REQUIRE(controller.activate(secondMove));
        REQUIRE(codeset.getErrorTable() == Codeset::getEmptyTable());

        // The second keyframe should start at laterTime, not chained from the first
        const auto it = std::find_if(builder.character.keyframes.begin(), builder.character.keyframes.end(),
            [](const Keyframe& kf){ return kf.animation == ANIMATION_WALKING_FROM_WALL_TO_FLOOR; });

        REQUIRE(it != builder.character.keyframes.end());
        REQUIRE(it->t0 == laterTime);
        REQUIRE(it->t1 == laterTime + MatchController::MOVE_ANIMATION_DURATION);
    });
}
