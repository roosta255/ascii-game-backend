#include <catch2/catch_test_macros.hpp>

#include "ActivationContext.hpp"
#include "ActivatorAlterTraitAffliction.hpp"
#include "Character.hpp"
#include "CharacterRelation.hpp"
#include "Codeset.hpp"
#include "DungeonRoleTransformer.hpp"
#include "Location.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Player.hpp"
#include "RequestContext.hpp"
#include "Room.hpp"
#include "RoleEnum.hpp"
#include "Timestamp.hpp"
#include "TraitEnum.hpp"

// ---- helpers ----

static Character& allocateDungeonCharacter(Match& match, MatchController& controller, RoleEnum role, Location location) {
    Character* ptr = nullptr;
    const bool ok = match.allocateCharacter([&](Character& ch) {
        ch.role = role;
        ch.location = location;
        ptr = &ch;
    });
    REQUIRE(ok);
    REQUIRE(ptr != nullptr);
    controller.updateTraits(*ptr);
    return *ptr;
}

static RequestContext makeRequest(Player& player, Match& match, Codeset& codeset, MatchController& controller) {
    return RequestContext{
        .player = player,
        .match = match,
        .codeset = codeset,
        .controller = controller,
        .time = Timestamp::nil(),
        .isSkippingAnimations = true,
    };
}

static ActivationContext makeActivation(RequestContext& request, Room& room, Character& actor) {
    return ActivationContext{
        .codeset = request.codeset,
        .request = request,
        .room = room,
        .character = actor,
        .time = request.time
    };
}

// ---- ActivationContext::resolveCharacter ----

TEST_CASE("resolveCharacter: Self returns the anchor unchanged", "[character-relation]") {
    Match match;
    Codeset codeset;
    MatchController controller(match, codeset);
    Player player;
    Room room;

    Character& actor = allocateDungeonCharacter(match, controller, ROLE_PROCESSOR, Location::makeFloor(0, CHANNEL_CORPOREAL, 0));

    RequestContext request = makeRequest(player, match, codeset, controller);
    ActivationContext activation = makeActivation(request, room, actor);

    Pointer<Character> resolved = activation.resolveCharacter(CharacterAnchor::Actor, CharacterRelation::Self);
    REQUIRE(resolved.isPresent());
    resolved.access([&](Character& ch) { REQUIRE(ch.characterId == actor.characterId); });
}

TEST_CASE("resolveCharacter: finds the paired role in the same room", "[character-relation]") {
    Match match;
    Codeset codeset;
    MatchController controller(match, codeset);
    Player player;
    Room room;

    Character& processor  = allocateDungeonCharacter(match, controller, ROLE_PROCESSOR,  Location::makeFloor(0, CHANNEL_CORPOREAL, 0));
    Character& receptacle = allocateDungeonCharacter(match, controller, ROLE_RECEPTACLE, Location::makeFloor(0, CHANNEL_CORPOREAL, 1));

    RequestContext request = makeRequest(player, match, codeset, controller);
    ActivationContext activation = makeActivation(request, room, processor);

    Pointer<Character> resolved = activation.resolveCharacter(CharacterAnchor::Actor, CharacterRelation::Secondary);
    REQUIRE(resolved.isPresent());
    resolved.access([&](Character& ch) { REQUIRE(ch.characterId == receptacle.characterId); });
}

TEST_CASE("resolveCharacter: paired lookup is symmetric (Receptacle -> Primary finds Processor)", "[character-relation]") {
    Match match;
    Codeset codeset;
    MatchController controller(match, codeset);
    Player player;
    Room room;

    Character& processor  = allocateDungeonCharacter(match, controller, ROLE_PROCESSOR,  Location::makeFloor(0, CHANNEL_CORPOREAL, 0));
    Character& receptacle = allocateDungeonCharacter(match, controller, ROLE_RECEPTACLE, Location::makeFloor(0, CHANNEL_CORPOREAL, 1));

    RequestContext request = makeRequest(player, match, codeset, controller);
    ActivationContext activation = makeActivation(request, room, receptacle);

    Pointer<Character> resolved = activation.resolveCharacter(CharacterAnchor::Actor, CharacterRelation::Primary);
    REQUIRE(resolved.isPresent());
    resolved.access([&](Character& ch) { REQUIRE(ch.characterId == processor.characterId); });
}

TEST_CASE("resolveCharacter: ignores a paired role in a different room", "[character-relation]") {
    Match match;
    Codeset codeset;
    MatchController controller(match, codeset);
    Player player;
    Room room;

    Character& processor = allocateDungeonCharacter(match, controller, ROLE_PROCESSOR, Location::makeFloor(0, CHANNEL_CORPOREAL, 0));
    allocateDungeonCharacter(match, controller, ROLE_RECEPTACLE, Location::makeFloor(1, CHANNEL_CORPOREAL, 0));

    RequestContext request = makeRequest(player, match, codeset, controller);
    ActivationContext activation = makeActivation(request, room, processor);

    Pointer<Character> resolved = activation.resolveCharacter(CharacterAnchor::Actor, CharacterRelation::Secondary);
    REQUIRE(resolved.isEmpty());
}

TEST_CASE("resolveCharacter: chest-contained anchor only searches its own chest", "[character-relation]") {
    Match match;
    Codeset codeset;
    MatchController controller(match, codeset);
    Player player;
    Room room;

    // container ids are arbitrary here — only used to key chest membership.
    Character& processorInChestA  = allocateDungeonCharacter(match, controller, ROLE_PROCESSOR,  Location::makeChest(0, CHANNEL_CORPOREAL, 100));
    Character& receptacleInChestA = allocateDungeonCharacter(match, controller, ROLE_RECEPTACLE, Location::makeChest(0, CHANNEL_CORPOREAL, 100));
    allocateDungeonCharacter(match, controller, ROLE_RECEPTACLE, Location::makeChest(0, CHANNEL_CORPOREAL, 200));

    RequestContext request = makeRequest(player, match, codeset, controller);
    ActivationContext activation = makeActivation(request, room, processorInChestA);

    Pointer<Character> resolved = activation.resolveCharacter(CharacterAnchor::Actor, CharacterRelation::Secondary);
    REQUIRE(resolved.isPresent());
    resolved.access([&](Character& ch) { REQUIRE(ch.characterId == receptacleInChestA.characterId); });
}

TEST_CASE("resolveCharacter: Target anchor resolves relative to the target character", "[character-relation]") {
    Match match;
    Codeset codeset;
    MatchController controller(match, codeset);
    Player player;
    Room room;

    Character& actor = allocateDungeonCharacter(match, controller, ROLE_BUILDER, Location::makeFloor(0, CHANNEL_CORPOREAL, 0));
    Character& processor  = allocateDungeonCharacter(match, controller, ROLE_PROCESSOR,  Location::makeFloor(0, CHANNEL_CORPOREAL, 1));
    Character& receptacle = allocateDungeonCharacter(match, controller, ROLE_RECEPTACLE, Location::makeFloor(0, CHANNEL_CORPOREAL, 2));

    RequestContext request = makeRequest(player, match, codeset, controller);
    ActivationContext activation = makeActivation(request, room, actor);
    activation.targetEntity = processor;

    Pointer<Character> resolved = activation.resolveCharacter(CharacterAnchor::Target, CharacterRelation::Secondary);
    REQUIRE(resolved.isPresent());
    resolved.access([&](Character& ch) { REQUIRE(ch.characterId == receptacle.characterId); });
}

// ---- DungeonRoleTransformer with Anchor/Relation ----

TEST_CASE("DungeonRoleTransformer: Anchor/Relation transforms the paired character, not the actor", "[character-relation]") {
    Match match;
    Codeset codeset;
    MatchController controller(match, codeset);
    Player player;
    Room room;

    Character& processor  = allocateDungeonCharacter(match, controller, ROLE_PROCESSOR,  Location::makeFloor(0, CHANNEL_CORPOREAL, 0));
    Character& receptacle = allocateDungeonCharacter(match, controller, ROLE_RECEPTACLE, Location::makeFloor(0, CHANNEL_CORPOREAL, 1));

    RequestContext request = makeRequest(player, match, codeset, controller);
    ActivationContext activation = makeActivation(request, room, processor);

    DungeonRoleTransformer transformer(
        CharacterAnchor::Actor,
        CharacterRelation::Secondary,
        RoleAssignment{ .directRole = ROLE_EMPTY }
    );

    const bool ok = transformer.activate(activation);

    REQUIRE(ok);
    REQUIRE(processor.role == ROLE_PROCESSOR);   // actor untouched
    REQUIRE(receptacle.role == ROLE_EMPTY);       // paired character transformed
}

// ---- ActivatorAlterTraitAffliction with a related spec ----

TEST_CASE("ActivatorAlterTraitAffliction: related spec afflicts the paired character", "[character-relation]") {
    Match match;
    Codeset codeset;
    MatchController controller(match, codeset);
    Player player;
    Room room;

    Character& processor  = allocateDungeonCharacter(match, controller, ROLE_PROCESSOR,  Location::makeFloor(0, CHANNEL_CORPOREAL, 0));
    Character& receptacle = allocateDungeonCharacter(match, controller, ROLE_RECEPTACLE, Location::makeFloor(0, CHANNEL_CORPOREAL, 1));

    RequestContext request = makeRequest(player, match, codeset, controller);
    ActivationContext activation = makeActivation(request, room, processor);

    AlterTraitAfflictionConfig config;
    config.related = AlterTraitAfflictionRelatedSpec{
        .anchor = CharacterAnchor::Actor,
        .relation = CharacterRelation::Secondary,
        .spec = AlterTraitAfflictionSpec{ .set = makeTraitBits({ TRAIT_ASLEEP }) }
    };
    ActivatorAlterTraitAffliction activator(config);

    const bool ok = activator.activate(activation);

    REQUIRE(ok);
    REQUIRE(receptacle.traitsAfflicted[(size_t)TRAIT_ASLEEP].orElse(false));
    REQUIRE_FALSE(processor.traitsAfflicted[(size_t)TRAIT_ASLEEP].orElse(false));
}
