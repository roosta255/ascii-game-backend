#include <catch2/catch_test_macros.hpp>
#include "Codeset.hpp"
#include "DUNGEON_ROOM_COUNT.hpp"
#include "FieldEnum.hpp"
#include "FieldFlyweight.hpp"
#include "Match.hpp"
#include "MatchController.hpp"

TEST_CASE("FieldFlyweight declares OXYGEN as the only field", "[field]") {
    REQUIRE(FIELD_COUNT == 1);
    for (const FieldFlyweight& flyweight : FieldFlyweight::getFlyweights()) {
        REQUIRE(flyweight.name == "OXYGEN");
        REQUIRE(flyweight.minValue == 0);
        REQUIRE(flyweight.maxValue == 100);
    }
}

TEST_CASE("MatchController recomputes oxygen for every room on construction", "[field]") {
    Match match;
    Codeset codeset;
    MatchController controller(match, codeset);

    for (int roomId = 0; roomId < DUNGEON_ROOM_COUNT; roomId++) {
        int16_t oxygen = controller.getFieldValue(FIELD_OXYGEN, roomId);
        REQUIRE(oxygen >= 0);
        REQUIRE(oxygen <= 100);
    }
}
