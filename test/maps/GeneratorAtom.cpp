#include <catch2/catch_test_macros.hpp>
#include "Codeset.hpp"
#include "GeneratorAtom.hpp"
#include "GeneratorEnum.hpp"
#include "LocationEnum.hpp"
#include "Match.hpp"
#include "TestController.hpp"

TEST_CASE("GeneratorAtom: generates without errors", "[match][GENERATOR_ATOM]") {
    TestController tc(GENERATOR_ATOM);

    tc.generate(GeneratorAtom::CANONICAL_SEED);

    REQUIRE(tc.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(tc.isSuccess);
}

TEST_CASE("GeneratorAtom: match starts and player is placed", "[match][GENERATOR_ATOM]") {
    TestController tc(GENERATOR_ATOM);

    tc.generate(GeneratorAtom::CANONICAL_SEED);
    REQUIRE(tc.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(tc.isSuccess);

    REQUIRE(tc.match.start());
    REQUIRE(tc.codeset.getErrorTable() == Codeset::getEmptyTable());

    REQUIRE(tc.latestPosition == GeneratorAtom::START_ROOM);
    REQUIRE(tc.builderCharacterPtr != nullptr);
    REQUIRE(tc.builderCharacterPtr->location.type == LOCATION_FLOOR);
}

TEST_CASE("GeneratorAtom: deterministic across two runs with same seed", "[match][GENERATOR_ATOM]") {
    TestController tc1(GENERATOR_ATOM);
    tc1.generate(GeneratorAtom::CANONICAL_SEED);
    REQUIRE(tc1.codeset.getErrorTable() == Codeset::getEmptyTable());

    TestController tc2(GENERATOR_ATOM);
    tc2.generate(GeneratorAtom::CANONICAL_SEED);
    REQUIRE(tc2.codeset.getErrorTable() == Codeset::getEmptyTable());

    REQUIRE(tc1.match == tc2.match);
}

TEST_CASE("GeneratorAtom: different seeds produce different dungeons", "[match][GENERATOR_ATOM]") {
    TestController tc1(GENERATOR_ATOM);
    tc1.generate(0);
    REQUIRE(tc1.codeset.getErrorTable() == Codeset::getEmptyTable());

    TestController tc2(GENERATOR_ATOM);
    tc2.generate(1);
    REQUIRE(tc2.codeset.getErrorTable() == Codeset::getEmptyTable());

    REQUIRE_FALSE(tc1.match == tc2.match);
}

TEST_CASE("GeneratorAtom: start room exists", "[match][GENERATOR_ATOM]") {
    TestController tc(GENERATOR_ATOM);
    tc.generate(GeneratorAtom::CANONICAL_SEED);

    REQUIRE(tc.isSuccess);
    REQUIRE(tc.match.start());

    REQUIRE(tc.latestPosition == GeneratorAtom::START_ROOM);
}

TEST_CASE("GeneratorAtom: exit door exists", "[match][GENERATOR_ATOM]") {
    TestController tc(GENERATOR_ATOM);
    tc.generate(GeneratorAtom::CANONICAL_SEED);

    bool foundExit = tc.match.dungeon.rooms.isAny([](const Room& room) {
        return room.walls.isAny([](const Wall& wall) { return wall.door == DOOR_EXIT; });
    });

    REQUIRE(foundExit);
}

TEST_CASE("GeneratorAtom: deterministic across repeated runs", "[match][GENERATOR_ATOM]") {
    TestController baseline(GENERATOR_ATOM);
    baseline.generate(12345);

    for (int i = 0; i < 10; ++i) {
        TestController trial(GENERATOR_ATOM);
        trial.generate(12345);
        REQUIRE(trial.match == baseline.match);
    }
}

TEST_CASE("GeneratorAtom: many seeds generate successfully", "[match][GENERATOR_ATOM]") {
    for (int seed = 0; seed < 100; ++seed) {
        TestController tc(GENERATOR_ATOM);
        tc.generate(seed);

        REQUIRE(tc.isSuccess);
        REQUIRE(tc.codeset.getErrorTable() == Codeset::getEmptyTable());
    }
}

TEST_CASE("GeneratorAtom: seed diversity", "[match][GENERATOR_ATOM]") {
    TestController a(GENERATOR_ATOM);
    TestController b(GENERATOR_ATOM);
    TestController c(GENERATOR_ATOM);

    a.generate(1);
    b.generate(2);
    c.generate(3);

    REQUIRE_FALSE(a.match == b.match);
    REQUIRE_FALSE(a.match == c.match);
}

TEST_CASE("GeneratorAtom: generated matches can start", "[match][GENERATOR_ATOM]") {
    for (int seed = 0; seed < 25; ++seed) {
        TestController tc(GENERATOR_ATOM);
        tc.generate(seed);

        REQUIRE(tc.match.start());
        REQUIRE(tc.codeset.getErrorTable() == Codeset::getEmptyTable());
    }
}

TEST_CASE("GeneratorAtom: builder always placed", "[match][GENERATOR_ATOM]") {
    for (int seed = 0; seed < 25; ++seed) {
        TestController tc(GENERATOR_ATOM);
        tc.generate(seed);
        tc.match.start();

        REQUIRE(tc.builderCharacterPtr != nullptr);
    }
}

TEST_CASE("GeneratorAtom: dungeon has interactable content", "[match][GENERATOR_ATOM]") {
    TestController tc(GENERATOR_ATOM);
    tc.generate(1);

    int nonEmpty = 0;

    for (int i = 0; i < tc.match.dungeon.rooms.size(); ++i) {
        tc.match.dungeon.rooms.accessConst(i, [&](const Room& room) {
            if (room.roomId != -1)
                ++nonEmpty;
        });
    }

    REQUIRE(nonEmpty > 0);
}

TEST_CASE("GeneratorAtom: canonical seed remains stable", "[match][GENERATOR_ATOM]") {
    TestController tc(GENERATOR_ATOM);
    tc.generate(GeneratorAtom::CANONICAL_SEED);

    TestController tc2(GENERATOR_ATOM);
    tc2.generate(GeneratorAtom::CANONICAL_SEED);

    REQUIRE(std::hash<Match>{}(tc.match) == std::hash<Match>{}(tc2.match));
}

TEST_CASE("GeneratorAtom: fuzz generation", "[match][GENERATOR_ATOM][slow]") {
    for (int seed = 0; seed < 1000; ++seed) {
        TestController tc(GENERATOR_ATOM);
        tc.generate(seed);

        REQUIRE(tc.isSuccess);
    }
}
