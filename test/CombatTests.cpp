#include <catch2/catch_test_macros.hpp>

#include "ActivatorAttack.hpp"
#include "ActivatorDamage.hpp"
#include "Activation.hpp"
#include "Character.hpp"
#include "Codeset.hpp"
#include "DamageTypeBits.hpp"
#include "DamageTypeEnum.hpp"
#include "GeneratorEnum.hpp"
#include "ItemEnum.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Player.hpp"
#include "RequestContext.hpp"
#include "Room.hpp"
#include "RoleEnum.hpp"
#include "TestController.hpp"
#include "Timestamp.hpp"
#include "TraitEnum.hpp"

// ---- helpers ----

// Allocate a dungeon character with the given role and register its computed traits.
static Character& allocateDungeonCharacter(Match& match, MatchController& controller, RoleEnum role) {
    Character* ptr = nullptr;
    const bool ok = match.allocateCharacter([&](Character& ch) {
        ch.role = role;
        ptr = &ch;
    });
    REQUIRE(ok);
    REQUIRE(ptr != nullptr);
    controller.updateTraits(*ptr);
    return *ptr;
}

// Build a minimal RequestContext for direct activator testing.
static RequestContext makeRequest(
    Player& player,
    Room& room,
    Match& match,
    Codeset& codeset,
    MatchController& controller)
{
    return RequestContext{
        .player = player,
        .room = room,
        .match = match,
        .codeset = codeset,
        .controller = controller,
        .time = Timestamp::nil(),
        .isSkippingAnimations = true,
    };
}

// Build a minimal Activation for direct activator testing.
static Activation makeActivation(
    RequestContext& request,
    Character& attacker,
    Character& target,
    DamageTypeBits damageTypes = {})
{
    return Activation{
        .request = &request,
        .character = attacker,
        .target = Pointer<Character>(target),
        .damageTypes = damageTypes,
    };
}

// ---- attack tests (ActivatorAttack) ----

TEST_CASE("Combat: CAT attacks CAT - 1 damage", "[combat]") {
    Match match;
    Codeset codeset;
    MatchController controller(match, codeset);
    Player player;
    Room room;

    Character& attacker = allocateDungeonCharacter(match, controller, ROLE_CAT);
    Character& target   = allocateDungeonCharacter(match, controller, ROLE_CAT);

    RequestContext request = makeRequest(player, room, match, codeset, controller);
    Activation activation = makeActivation(request, attacker, target);

    ActivatorAttack attackActivator;
    const bool ok = attackActivator.activate(activation);

    REQUIRE(ok);
    REQUIRE(target.damage == 1);
}

TEST_CASE("Combat: CAT attacks MELEE-resistant CAT - 0 damage (vector resisted)", "[combat]") {
    Match match;
    Codeset codeset;
    MatchController controller(match, codeset);
    Player player;
    Room room;

    Character& attacker = allocateDungeonCharacter(match, controller, ROLE_CAT);
    Character& target   = allocateDungeonCharacter(match, controller, ROLE_CAT);

    // All melee vectors will be resisted → early return with 0 damage
    target.traitsAfflicted.setIndexOn(TRAIT_DAMAGE_RESIST_MELEE);
    controller.updateTraits(target);

    RequestContext request = makeRequest(player, room, match, codeset, controller);
    Activation activation = makeActivation(request, attacker, target);

    ActivatorAttack attackActivator;
    const bool ok = attackActivator.activate(activation);

    REQUIRE(ok);
    REQUIRE(target.damage == 0);
}

TEST_CASE("Combat: CAT attacks SLASH-resistant CAT - 0 damage (all effects resisted)", "[combat]") {
    Match match;
    Codeset codeset;
    MatchController controller(match, codeset);
    Player player;
    Room room;

    Character& attacker = allocateDungeonCharacter(match, controller, ROLE_CAT);
    Character& target   = allocateDungeonCharacter(match, controller, ROLE_CAT);

    // CAT deals SLASH as its only effect; resisting it zeroes out base damage
    target.traitsAfflicted.setIndexOn(TRAIT_DAMAGE_RESIST_SLASH);
    controller.updateTraits(target);

    RequestContext request = makeRequest(player, room, match, codeset, controller);
    Activation activation = makeActivation(request, attacker, target);

    ActivatorAttack attackActivator;
    const bool ok = attackActivator.activate(activation);

    REQUIRE(ok);
    REQUIRE(target.damage == 0);
}

// ---- direct damage tests (ActivatorDamage) ----

TEST_CASE("Combat: FIRE damage - 2 damage (1 base + 1 fire bonus)", "[combat]") {
    Match match;
    Codeset codeset;
    MatchController controller(match, codeset);
    Player player;
    Room room;

    Character& attacker = allocateDungeonCharacter(match, controller, ROLE_CAT);
    Character& target   = allocateDungeonCharacter(match, controller, ROLE_CAT);

    // No vector — skip straight to effect processing
    const auto damageTypes = makeDamageTypeBits({ DAMAGE_TYPE_FIRE });
    RequestContext request = makeRequest(player, room, match, codeset, controller);
    Activation activation = makeActivation(request, attacker, target, damageTypes);

    ActivatorDamage damageActivator;
    const bool ok = damageActivator.activate(activation);

    REQUIRE(ok);
    REQUIRE(target.damage == 2); // 1 base + 1 from ActivatorDamageFire
}

TEST_CASE("Combat: FIRE damage on FIRE-resistant target - 0 damage", "[combat]") {
    Match match;
    Codeset codeset;
    MatchController controller(match, codeset);
    Player player;
    Room room;

    Character& attacker = allocateDungeonCharacter(match, controller, ROLE_CAT);
    Character& target   = allocateDungeonCharacter(match, controller, ROLE_CAT);

    target.traitsAfflicted.setIndexOn(TRAIT_DAMAGE_RESIST_FIRE);
    controller.updateTraits(target);

    const auto damageTypes = makeDamageTypeBits({ DAMAGE_TYPE_FIRE });
    RequestContext request = makeRequest(player, room, match, codeset, controller);
    Activation activation = makeActivation(request, attacker, target, damageTypes);

    ActivatorDamage damageActivator;
    const bool ok = damageActivator.activate(activation);

    REQUIRE(ok);
    REQUIRE(target.damage == 0);
}

TEST_CASE("Combat: PIERCE damage on PIERCE-weak target - 2 damage", "[combat]") {
    Match match;
    Codeset codeset;
    MatchController controller(match, codeset);
    Player player;
    Room room;

    Character& attacker = allocateDungeonCharacter(match, controller, ROLE_CAT);
    Character& target   = allocateDungeonCharacter(match, controller, ROLE_CAT);

    target.traitsAfflicted.setIndexOn(TRAIT_DAMAGE_WEAKNESS_PIERCE);
    controller.updateTraits(target);

    const auto damageTypes = makeDamageTypeBits({ DAMAGE_TYPE_PIERCE });
    RequestContext request = makeRequest(player, room, match, codeset, controller);
    Activation activation = makeActivation(request, attacker, target, damageTypes);

    ActivatorDamage damageActivator;
    const bool ok = damageActivator.activate(activation);

    REQUIRE(ok);
    REQUIRE(target.damage == 2); // 1 base + 1 from pierce weakness
}

TEST_CASE("Combat: PIERCE damage on PIERCE-resistant target - 0 damage", "[combat]") {
    Match match;
    Codeset codeset;
    MatchController controller(match, codeset);
    Player player;
    Room room;

    Character& attacker = allocateDungeonCharacter(match, controller, ROLE_CAT);
    Character& target   = allocateDungeonCharacter(match, controller, ROLE_CAT);

    target.traitsAfflicted.setIndexOn(TRAIT_DAMAGE_RESIST_PIERCE);
    controller.updateTraits(target);

    const auto damageTypes = makeDamageTypeBits({ DAMAGE_TYPE_PIERCE });
    RequestContext request = makeRequest(player, room, match, codeset, controller);
    Activation activation = makeActivation(request, attacker, target, damageTypes);

    ActivatorDamage damageActivator;
    const bool ok = damageActivator.activate(activation);

    REQUIRE(ok);
    REQUIRE(target.damage == 0);
}

TEST_CASE("Combat: COLD damage - 1 damage and target loses a move", "[combat]") {
    Match match;
    Codeset codeset;
    MatchController controller(match, codeset);
    Player player;
    Room room;

    Character& attacker = allocateDungeonCharacter(match, controller, ROLE_CAT);
    Character& target   = allocateDungeonCharacter(match, controller, ROLE_CAT);
    // target.moves == 0 and CAT flyweight.moves == 1 → MOVEMENT_READY is computed as true

    const auto damageTypes = makeDamageTypeBits({ DAMAGE_TYPE_COLD });
    RequestContext request = makeRequest(player, room, match, codeset, controller);
    Activation activation = makeActivation(request, attacker, target, damageTypes);

    ActivatorDamage damageActivator;
    const bool ok = damageActivator.activate(activation);

    REQUIRE(ok);
    REQUIRE(target.damage == 1);
    // takeCharacterMove increments moves; CAT flyweight.moves == 1 so the move is now spent
    REQUIRE(target.moves == 1);
}

TEST_CASE("Combat: ELECTRIC damage - 1 damage and target loses an action", "[combat]") {
    Match match;
    Codeset codeset;
    MatchController controller(match, codeset);
    Player player;
    Room room;

    Character& attacker = allocateDungeonCharacter(match, controller, ROLE_CAT);
    Character& target   = allocateDungeonCharacter(match, controller, ROLE_CAT);
    // target.actions == 0 and CAT flyweight.actions == 1 → ACTION_READY is computed as true

    const auto damageTypes = makeDamageTypeBits({ DAMAGE_TYPE_ELECTRIC });
    RequestContext request = makeRequest(player, room, match, codeset, controller);
    Activation activation = makeActivation(request, attacker, target, damageTypes);

    ActivatorDamage damageActivator;
    const bool ok = damageActivator.activate(activation);

    REQUIRE(ok);
    REQUIRE(target.damage == 1);
    // takeCharacterAction increments actions; CAT flyweight.actions == 1 so the action is now spent
    REQUIRE(target.actions == 1);
}

TEST_CASE("Combat: POISE negates base damage when action-ready", "[combat]") {
    Match match;
    Codeset codeset;
    MatchController controller(match, codeset);
    Player player;
    Room room;

    Character& attacker = allocateDungeonCharacter(match, controller, ROLE_CAT);
    Character& target   = allocateDungeonCharacter(match, controller, ROLE_CAT);

    // POISE + ACTION_READY (target.actions==0, flyweight.actions==1) → baseDamage negated.
    // SLASH is noop so no per-effect bonus either.
    target.traitsAfflicted.setIndexOn(TRAIT_POISE);
    controller.updateTraits(target);

    RequestContext request = makeRequest(player, room, match, codeset, controller);
    Activation activation = makeActivation(request, attacker, target);

    ActivatorAttack attackActivator;
    const bool ok = attackActivator.activate(activation);

    REQUIRE(ok);
    REQUIRE(target.damage == 0);
}

TEST_CASE("Combat: PARRY triggers counter-attack", "[combat]") {
    Match match;
    Codeset codeset;
    MatchController controller(match, codeset);
    Player player;
    Room room;

    Character& attacker = allocateDungeonCharacter(match, controller, ROLE_CAT);
    Character& target   = allocateDungeonCharacter(match, controller, ROLE_CAT);

    // PARRY + ACTION_READY + MOVEMENT_READY on the target causes a counter-attack.
    // Both are computed from counts==0 < flyweight counts (CAT has 1 action, 1 move).
    target.traitsAfflicted.setIndexOn(TRAIT_PARRY);
    controller.updateTraits(target);

    RequestContext request = makeRequest(player, room, match, codeset, controller);
    Activation activation = makeActivation(request, attacker, target);

    ActivatorAttack attackActivator;
    const bool ok = attackActivator.activate(activation);

    REQUIRE(ok);
    // Counter-attack: target spends one action and one move to parry,
    // then attacks the original attacker for 1 damage (CAT → MELEE+SLASH, no resistances).
    REQUIRE(target.actions == 1);  // takeCharacterAction was called
    REQUIRE(target.moves  == 1);   // takeCharacterMove was called
    REQUIRE(attacker.damage == 1); // counter-attack landed
    REQUIRE(target.damage   == 0); // original attack was cancelled
}

TEST_CASE("Combat: CRUSH damage breaks builder armor", "[combat]") {
    // breakArmorItem searches match.builders, so the target must be a builder character.
    TestController tc(GENERATOR_TUTORIAL);

    tc.giveItem(ITEM_ARMOR);

    // Verify the armor was placed in the builder's inventory
    bool hadArmor = false;
    tc.inventoryPtr->accessItem(ITEM_ARMOR, [&](const Item& item) {
        hadArmor = item.stacks > 0;
    });
    REQUIRE(hadArmor);

    Character& builderChar = *tc.builderCharacterPtr;
    tc.controller.updateTraits(builderChar);

    // Attacker role does not matter for a direct ActivatorDamage call
    Character& attacker = allocateDungeonCharacter(tc.match, tc.controller, ROLE_CAT);

    Player player;
    Room room;

    const auto damageTypes = makeDamageTypeBits({ DAMAGE_TYPE_CRUSH });
    RequestContext request = makeRequest(player, room, tc.match, tc.codeset, tc.controller);
    Activation activation = makeActivation(request, attacker, builderChar, damageTypes);

    ActivatorDamage damageActivator;
    const bool ok = damageActivator.activate(activation);

    REQUIRE(ok);

    // Armor should be gone after crush
    bool stillHasArmor = false;
    tc.inventoryPtr->accessItem(ITEM_ARMOR, [&](const Item& item) {
        stillHasArmor = item.stacks > 0;
    });
    REQUIRE(!stillHasArmor);
}
