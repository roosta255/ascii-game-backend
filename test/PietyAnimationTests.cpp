#include <catch2/catch_test_macros.hpp>
#include "AnimationEnum.hpp"
#include "Character.hpp"
#include "CharacterAllocSpec.hpp"
#include "ChannelEnum.hpp"
#include "GeneratorEnum.hpp"
#include "Keyframe.hpp"
#include "Preactivation.hpp"
#include "Rack.hpp"
#include "RoleEnum.hpp"
#include "TestController.hpp"
#include "TraitEnum.hpp"

static Maybe<Keyframe> findKeyframe(const Array<Keyframe, Character::MAX_KEYFRAMES>& keyframes, AnimationEnum animation) {
    for (const auto& kf : keyframes) {
        if (kf.animation == animation) return Maybe<Keyframe>(kf);
    }
    return Maybe<Keyframe>::empty();
}

TEST_CASE("SACRAMENT_FORGIVENESS grants PIETY and inserts eye color keyframe", "[traits][animation][piety]") {
    TestController tc(GENERATOR_TUTORIAL);
    tc.generate(0);
    REQUIRE(tc.match.start());
    tc.endTurn();
    REQUIRE(tc.isSuccess);

    // Place a SACRAMENT_FORGIVENESS character in the builder's current room
    int sacramentId = -1;
    bool allocated = tc.controller.allocate(
        CharacterAllocSpec{.role = ROLE_SACRAMENT_FORGIVENESS},
        AttachmentContext::floor(tc.latestPosition),
        sacramentId
    );
    REQUIRE(allocated);
    REQUIRE(sacramentId != -1);

    // Builder should not have PIETY yet
    REQUIRE_FALSE(tc.controller.getTraitsComputed(tc.builderOffset).final[TRAIT_PIETY].orElse(false));

    // Activate the sacrament (actor=sacrament, target=builder)
    Preactivation preactivation{
        .action = {
            .type = ACTION_ACTIVATE_CHARACTER,
            .characterId = sacramentId,
            .roomId = tc.latestPosition,
            .targetCharacterId = tc.builderOffset,
        },
        .playerId = TestController::BUILDER_ID,
        .isSkippingAnimations = false,
    };
    REQUIRE(tc.controller.activate(preactivation));

    // Builder now has PIETY trait
    REQUIRE(tc.controller.getTraitsComputed(tc.builderOffset).final[TRAIT_PIETY].orElse(false));

    // Builder has a PIETY eye color keyframe with begins=true (data: from=0, to=1)
    auto kf = findKeyframe(tc.builderCharacterPtr->keyframes, ANIMATION_PIETY);
    REQUIRE(kf.isPresent());
    kf.accessConst([](const Keyframe& k) {
        REQUIRE(k.data.begin()[0] == 0);
        REQUIRE(k.data.begin()[1] == 1);
    });
}

TEST_CASE("SACRAMENT_FORGIVENESS skips PIETY keyframe when animations are disabled", "[traits][animation][piety]") {
    TestController tc(GENERATOR_TUTORIAL);
    tc.generate(0);
    REQUIRE(tc.match.start());
    tc.endTurn();
    REQUIRE(tc.isSuccess);

    int sacramentId = -1;
    REQUIRE(tc.controller.allocate(
        CharacterAllocSpec{.role = ROLE_SACRAMENT_FORGIVENESS},
        AttachmentContext::floor(tc.latestPosition),
        sacramentId
    ));

    Preactivation preactivation{
        .action = {
            .type = ACTION_ACTIVATE_CHARACTER,
            .characterId = sacramentId,
            .roomId = tc.latestPosition,
            .targetCharacterId = tc.builderOffset,
        },
        .playerId = TestController::BUILDER_ID,
        .isSkippingAnimations = true,
    };
    REQUIRE(tc.controller.activate(preactivation));

    REQUIRE(tc.controller.getTraitsComputed(tc.builderOffset).final[TRAIT_PIETY].orElse(false));
    REQUIRE_FALSE(findKeyframe(tc.builderCharacterPtr->keyframes, ANIMATION_PIETY).isPresent());
}
