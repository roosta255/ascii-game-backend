#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include "Cardinal.hpp"
#include "Character.hpp"
#include "CodeEnum.hpp"
#include "Codeset.hpp"
#include "Dungeon.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "GeneratorEnum.hpp"
#include "GeneratorTutorial.hpp"
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
        isSuccess = controller.moveCharacterToWall(0, builderOffset, Cardinal::east(), t0);
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
