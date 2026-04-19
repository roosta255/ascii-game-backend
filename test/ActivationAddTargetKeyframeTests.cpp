#include <catch2/catch_test_macros.hpp>
#include "ActivationAddTargetKeyframe.hpp"
#include "ActivationContext.hpp"
#include "AnimationConfig.hpp"
#include "AnimationEnum.hpp"
#include "Cardinal.hpp"
#include "Character.hpp"
#include "ChannelEnum.hpp"
#include "Codeset.hpp"
#include "GeneratorEnum.hpp"
#include "Keyframe.hpp"
#include "Location.hpp"
#include "Preactivation.hpp"
#include "Rack.hpp"
#include "RoleEnum.hpp"
#include "TestController.hpp"

static Maybe<Keyframe> queryKeyframes(Rack<Keyframe> keyframes, AnimationEnum animation) {
    Maybe<Keyframe> latest = Maybe<Keyframe>::empty();
    for (const auto& kf : keyframes) {
        if (kf.animation == animation) {
            if (latest.isEmpty() || kf.t0 > latest.orElse(Keyframe{}).t0) {
                latest = Maybe<Keyframe>(kf);
            }
        }
    }
    return latest;
}

TEST_CASE("ActivationAddTargetKeyframe inserts into target character keyframes", "[activation][keyframe]") {
    TestController controller(GENERATOR_TUTORIAL);
    controller.generate(0);
    REQUIRE(controller.match.start());

    // Move builder east to the shared door between room 0 and room 1
    controller.moveCharacterToWall(Cardinal::east());
    REQUIRE(controller.isSuccess);
    REQUIRE(controller.builderCharacterPtr->location == Location::makeSharedDoor(1, CHANNEL_CORPOREAL, Cardinal::west()));

    // End the turn so the builder has full actions
    controller.endTurn();
    REQUIRE(controller.isSuccess);

    // Find the toggler in room 1
    int toggler1Offset = -1;
    REQUIRE(controller.match.findCharacter(toggler1Offset, [](const Character& c) {
        return c.role == ROLE_TOGGLER_BLUE && c.location.roomId == 1;
    }));
    REQUIRE(toggler1Offset != -1);

    // Build activation context with toggler as actor and builder as target
    Preactivation preactivation{
        .action = {
            .type = ACTION_ACTIVATE_CHARACTER,
            .characterId = toggler1Offset,
            .roomId = 1,
            .targetCharacterId = controller.builderOffset,
        },
        .playerId = TestController::BUILDER_ID,
        .isSkippingAnimations = false,
    };

    bool contextBuilt = false;
    controller.controller.buildActivationContext(preactivation, [&](ActivationContext& ctx) {
        contextBuilt = true;

        // Verify the actor is the toggler
        REQUIRE(ctx.character.role == ROLE_TOGGLER_BLUE);
        REQUIRE(ctx.character.location.roomId == 1);

        // Verify targetCharacter() returns the builder
        bool hasTarget = false;
        ctx.targetCharacter().access([&](Character& c) {
            hasTarget = true;
            REQUIRE(c.role == ROLE_BUILDER);
            REQUIRE(&c == controller.builderCharacterPtr);
        });
        REQUIRE(hasTarget);

        // Directly invoke the effect
        ActivationAddTargetKeyframe effect{{ANIMATION_BOUNCE_FROM_FLOOR_TO_LOCK, ANIMATION_ACT_TARGET_TO_SUBJECT}};
        bool effectResult = effect.activate(ctx);

        // Verify no errors and successful result
        REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
        REQUIRE(effectResult);

        // Verify the keyframe was inserted into the builder's keyframes (not the toggler's)
        auto builderRack = Rack<Keyframe>::buildFromArray<Character::MAX_KEYFRAMES>(controller.builderCharacterPtr->keyframes);
        REQUIRE(queryKeyframes(builderRack, ANIMATION_BOUNCE_FROM_DOOR_TO_FLOOR).isPresent());

        auto togglerRack = Rack<Keyframe>::buildFromArray<Character::MAX_KEYFRAMES>(ctx.character.keyframes);
        REQUIRE_FALSE(queryKeyframes(togglerRack, ANIMATION_BOUNCE_FROM_DOOR_TO_FLOOR).isPresent());
    });
    REQUIRE(contextBuilt);
}
