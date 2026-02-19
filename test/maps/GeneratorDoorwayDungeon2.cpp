#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include "Cardinal.hpp"
#include "Character.hpp"
#include "CharacterAction.hpp"
#include "CodeEnum.hpp"
#include "Codeset.hpp"
#include "DoorEnum.hpp"
#include "Dungeon.hpp"
#include "GeneratorDoorwayDungeon2.hpp"
#include "GeneratorEnum.hpp"
#include "int2.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Pathfinder.hpp"
#include "PathfindingCounter.hpp"
#include "RoleEnum.hpp"
#include "Room.hpp"
#include "TestController.hpp"

TEST_CASE("Test elevator configuration", "[match][GENERATOR_DOORWAY_DUNGEON_2]") {
    TestController controller(GENERATOR_DOORWAY_DUNGEON_2);
    auto& dungeon = controller.match.dungeon;

    // Generate test layout
    controller.controller.generate(0);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());

    const auto getElevatorDoorColumn = [&](){
        return GeneratorDoorwayDungeon2::ELEVATOR_COLUMN_ROOM_IDS.transform([&](const int index, const int& roomId){
            return dungeon.rooms.mapIndexAlaConst<DoorEnum>(roomId, [&](const Room& room){
                return DoorEnum(room.getWall(GeneratorDoorwayDungeon2::ELEVATOR_EXIT_DIRECTION.getFlip()).door);
            }).orElse(DOOR_COUNT);
        });
    };

    const auto getElevatorDoorEnums = [&](){
        return dungeon.rooms.mapIndexAlaConst<Array<DoorEnum, 4>>(GeneratorDoorwayDungeon2::ELEVATOR_ROOM_ID, [&](const Room& room){
            return room.walls.transform([&](const Wall& wall){
                return DoorEnum(wall.door);
            });
        }).orElse(Array<DoorEnum, 4>({DOOR_COUNT, DOOR_COUNT, DOOR_COUNT, DOOR_COUNT}));
    };

    const auto getElevatorRoomId = [&](){
        return dungeon.rooms.mapIndexAlaConst<int>(GeneratorDoorwayDungeon2::ELEVATOR_ROOM_ID, [&](const Room& room){
            return room.getWall(GeneratorDoorwayDungeon2::ELEVATOR_EXIT_DIRECTION).adjacent;
        }).orElse(-1);
    };

    REQUIRE(getElevatorDoorEnums() == std::array<DoorEnum, 4>
        { DOOR_ELEVATOR_CLOSED_KEYED
        , DOOR_ELEVATOR_CLOSED_KEYED
        , DOOR_ELEVATOR_OPEN_KEYED_BUTTON
        , DOOR_ELEVATOR_CLOSED_KEYED });
    REQUIRE(getElevatorRoomId() == GeneratorDoorwayDungeon2::ELEVATOR_COLUMN_ROOM_IDS.getOrDefault(6, -1));
    REQUIRE(getElevatorDoorColumn() == std::array<DoorEnum, 7>
        { DOOR_ELEVATOR_CLOSED_KEYLESS
        , DOOR_ELEVATOR_CLOSED_KEYLESS
        , DOOR_ELEVATOR_CLOSED_KEYLESS
        , DOOR_ELEVATOR_CLOSED_KEYLESS
        , DOOR_ELEVATOR_CLOSED_KEYLESS
        , DOOR_ELEVATOR_CLOSED_KEYLESS
        , DOOR_ELEVATOR_OPEN_KEYED });

    // count types
    /*
    const auto actualDoorCounts = countDoorsByType(controller.match);
    std::array<int, DOOR_COUNT> expectedDoorCounts{};
    expectedDoorCounts[DOOR_WALL] = 134;
    expectedDoorCounts[DOOR_DOORWAY] = 110;
    expectedDoorCounts[DOOR_LADDER_1_BOTTOM] = 2;
    expectedDoorCounts[DOOR_LADDER_1_TOP] = 2;
    expectedDoorCounts[DOOR_TOGGLER_BLUE_CLOSED] = 2;
    REQUIRE(actualDoorCounts == expectedDoorCounts);

    const auto actualRoleCounts = countCharactersByType(controller.match);
    std::array<int, ROLE_COUNT> expectedRoleCounts{};
    expectedRoleCounts[ROLE_TOGGLER] = 1;
    expectedRoleCounts[ROLE_BUILDER] = 1;
    REQUIRE(actualRoleCounts == expectedRoleCounts);
    REQUIRE(controller.codeset.describe() == "");
    */
}
