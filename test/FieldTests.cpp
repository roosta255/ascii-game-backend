#include <catch2/catch_test_macros.hpp>
#include "Cardinal.hpp"
#include "Codeset.hpp"
#include "DUNGEON_ROOM_COUNT.hpp"
#include "FieldController.hpp"
#include "FieldEnum.hpp"
#include "FieldFlyweight.hpp"
#include "Location.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "RoleEnum.hpp"
#include "RoomEnum.hpp"
#include "TraitEnum.hpp"
#include <string>

namespace {
    // Wires roomId's east wall to neighborId's west wall so the two rooms count as
    // adjacent for the neighbor-based resolvers (DISTANCE_FIELD, RELAXATION), the
    // same connection DungeonMutator sets up during real dungeon generation.
    void connectRoomsEastWest(Match& match, int roomId, int neighborId) {
        match.dungeon.rooms.access(roomId, [&](Room& room) {
            room.getWall(Cardinal::east()).adjacent = neighborId;
        });
        match.dungeon.rooms.access(neighborId, [&](Room& room) {
            room.getWall(Cardinal::west()).adjacent = roomId;
        });
    }

    void placeEnflamedCharacter(Match& match, int characterId, int roomId) {
        match.dungeon.characters.access(characterId, [&](Character& character) {
            character.role = ROLE_BUILDER;
            character.location = Location::makeFloor(roomId, CHANNEL_CORPOREAL, 0);
            character.traitsAfflicted.setIndexOn(TRAIT_ENFLAMED);
        });
    }
}

TEST_CASE("FieldFlyweight declares OXYGEN, HEAT, and SMOKE", "[field]") {
    REQUIRE(FIELD_COUNT == 3);

    std::string names[FIELD_COUNT];
    int i = 0;
    for (const FieldFlyweight& flyweight : FieldFlyweight::getFlyweights()) {
        names[i++] = flyweight.name;
    }
    REQUIRE(names[FIELD_OXYGEN] == "OXYGEN");
    REQUIRE(names[FIELD_HEAT] == "HEAT");
    REQUIRE(names[FIELD_SMOKE] == "SMOKE");

    FieldFlyweight::getFlyweights().accessConst(FIELD_OXYGEN, [&](const FieldFlyweight& flyweight) {
        REQUIRE(flyweight.minValue == 0);
        REQUIRE(flyweight.maxValue == 100);
    });
    FieldFlyweight::getFlyweights().accessConst(FIELD_HEAT, [&](const FieldFlyweight& flyweight) {
        REQUIRE(flyweight.minValue == -100);
        REQUIRE(flyweight.maxValue == 100);
    });
    FieldFlyweight::getFlyweights().accessConst(FIELD_SMOKE, [&](const FieldFlyweight& flyweight) {
        REQUIRE(flyweight.minValue == 0);
        REQUIRE(flyweight.maxValue == 255);
    });
}

TEST_CASE("MatchController recomputes every field for every room on construction", "[field]") {
    Match match;
    Codeset codeset;
    MatchController controller(match, codeset);

    for (int roomId = 0; roomId < DUNGEON_ROOM_COUNT; roomId++) {
        int16_t oxygen = controller.getFieldValue(FIELD_OXYGEN, roomId);
        REQUIRE(oxygen >= 0);
        REQUIRE(oxygen <= 100);

        int16_t heat = controller.getFieldValue(FIELD_HEAT, roomId);
        REQUIRE(heat >= -100);
        REQUIRE(heat <= 100);

        int16_t smoke = controller.getFieldValue(FIELD_SMOKE, roomId);
        REQUIRE(smoke >= 0);
        REQUIRE(smoke <= 255);
    }
}

TEST_CASE("OXYGEN: a vent room reads higher than an ambient room", "[field]") {
    Match match;
    match.dungeon.rooms.access(0, [&](Room& room) { room.type = ROOM_POWER_GENERATOR; });

    Codeset codeset;
    MatchController controller(match, codeset);

    REQUIRE(controller.getFieldValue(FIELD_OXYGEN, 0) > controller.getFieldValue(FIELD_OXYGEN, 1));
}

TEST_CASE("OXYGEN: an enflamed character consumes oxygen in their room", "[field]") {
    Match match;
    placeEnflamedCharacter(match, 0, 2);

    Codeset codeset;
    MatchController controller(match, codeset);

    REQUIRE(controller.getFieldValue(FIELD_OXYGEN, 2) < controller.getFieldValue(FIELD_OXYGEN, 3));
}

TEST_CASE("HEAT: a heat-source room radiates and decays into a connected neighbor", "[field]") {
    Match match;
    match.dungeon.rooms.access(0, [&](Room& room) { room.type = ROOM_LIGHTNING_ROD; });
    connectRoomsEastWest(match, 0, 1);

    Codeset codeset;
    MatchController controller(match, codeset);

    const int16_t sourceHeat = controller.getFieldValue(FIELD_HEAT, 0);
    const int16_t neighborHeat = controller.getFieldValue(FIELD_HEAT, 1);
    const int16_t unconnectedHeat = controller.getFieldValue(FIELD_HEAT, 2);

    REQUIRE(sourceHeat == 100);
    REQUIRE(neighborHeat > 0);
    REQUIRE(neighborHeat < sourceHeat);
    REQUIRE(unconnectedHeat == 0);
}

TEST_CASE("HEAT: an enflamed character is itself a heat source", "[field]") {
    Match match;
    placeEnflamedCharacter(match, 0, 5);

    Codeset codeset;
    MatchController controller(match, codeset);

    REQUIRE(controller.getFieldValue(FIELD_HEAT, 5) == 100);
}

TEST_CASE("SMOKE: an enflamed character fills their room with smoke and it diffuses to a neighbor", "[field]") {
    Match match;
    placeEnflamedCharacter(match, 0, 5);
    connectRoomsEastWest(match, 5, 6);

    Codeset codeset;
    MatchController controller(match, codeset);

    const int16_t sourceSmoke = controller.getFieldValue(FIELD_SMOKE, 5);
    const int16_t neighborSmoke = controller.getFieldValue(FIELD_SMOKE, 6);
    const int16_t unconnectedSmoke = controller.getFieldValue(FIELD_SMOKE, 7);

    REQUIRE(sourceSmoke > 0);
    REQUIRE(neighborSmoke > 0);
    REQUIRE(unconnectedSmoke == 0);
}

TEST_CASE("Character::endTurn accumulates hypoxiaTimer in a hypoxic room and resets elsewhere", "[field]") {
    Match match;
    // Two enflamed characters in room 0 drive its oxygen below the hypoxia threshold.
    placeEnflamedCharacter(match, 0, 0);
    placeEnflamedCharacter(match, 1, 0);
    match.dungeon.characters.access(2, [&](Character& character) {
        character.role = ROLE_BUILDER;
        character.location = Location::makeFloor(1, CHANNEL_CORPOREAL, 0);
    });

    REQUIRE(FieldController::isRoomHypoxic(match, 0));
    REQUIRE_FALSE(FieldController::isRoomHypoxic(match, 1));

    match.dungeon.characters.access(0, [&](Character& character) { character.endTurn(match); });
    match.dungeon.characters.access(0, [&](Character& character) { character.endTurn(match); });
    match.dungeon.characters.access(2, [&](Character& character) { character.endTurn(match); });

    match.dungeon.characters.accessConst(0, [&](const Character& character) {
        REQUIRE(character.hypoxiaTimer == 2);
    });
    match.dungeon.characters.accessConst(2, [&](const Character& character) {
        REQUIRE(character.hypoxiaTimer == 0);
    });

    // Clearing the room of enflamed characters restores oxygen; the timer resets.
    match.dungeon.characters.access(1, [&](Character& character) { character.traitsAfflicted.setIndexOff(TRAIT_ENFLAMED); });
    match.dungeon.characters.access(0, [&](Character& character) { character.endTurn(match); });
    match.dungeon.characters.accessConst(0, [&](const Character& character) {
        REQUIRE(character.hypoxiaTimer == 0);
    });
}
