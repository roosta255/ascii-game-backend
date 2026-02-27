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
    controller.generate(0);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // Start the match
    REQUIRE(controller.match.start());

    const auto getElevatorDoorColumn = [&](){
        return GeneratorDoorwayDungeon2::ELEVATOR_COLUMN_ROOM_IDS.transform([&](const int index, const int& roomId){
            return dungeon.rooms.mapIndexAlaConst<DoorEnum>(roomId, [&](const Room& room){
                return room.getWall(GeneratorDoorwayDungeon2::ELEVATOR_EXIT_DIRECTION.getFlip()).door;
            }).orElse(DOOR_COUNT);
        });
    };

    const auto getElevatorDoorEnums = [&](){
        return dungeon.rooms.mapIndexAlaConst<Array<DoorEnum, 4>>(GeneratorDoorwayDungeon2::ELEVATOR_ROOM_ID, [&](const Room& room){
            return room.walls.transform([&](const Wall& wall){
                return wall.door;
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

    controller.moveCharacterToWall(Cardinal::north());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.moveCharacterToWall(Cardinal::north());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.moveCharacterToWall(Cardinal::north());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // should be within elevator
    // give character a key
    REQUIRE(controller.inventory.elevatorKeys == 0);
    controller.giveItem(ITEM_KEY_ELEVATOR);
    REQUIRE(controller.isSuccess);
    REQUIRE(controller.inventory.elevatorKeys == 1);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // pay below floor 5 cost
    controller.activateLock(Cardinal::south());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);
    REQUIRE(controller.inventory.elevatorKeys == 0);
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
        , DOOR_ELEVATOR_CLOSED_CALL_BUTTON
        , DOOR_ELEVATOR_OPEN_KEYED });

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // move elevator down to floor 5
    controller.activateLock(Cardinal::south());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);
    REQUIRE(controller.inventory.elevatorKeys == 0);
    REQUIRE(getElevatorDoorEnums() == std::array<DoorEnum, 4>
        { DOOR_ELEVATOR_CLOSED_KEYED_BUTTON
        , DOOR_ELEVATOR_CLOSED_KEYED
        , DOOR_ELEVATOR_OPEN_KEYED_BUTTON
        , DOOR_ELEVATOR_CLOSED_KEYED });
    REQUIRE(getElevatorRoomId() == GeneratorDoorwayDungeon2::ELEVATOR_COLUMN_ROOM_IDS.getOrDefault(5, -1));
    REQUIRE(getElevatorDoorColumn() == std::array<DoorEnum, 7>
        { DOOR_ELEVATOR_CLOSED_KEYLESS
        , DOOR_ELEVATOR_CLOSED_KEYLESS
        , DOOR_ELEVATOR_CLOSED_KEYLESS
        , DOOR_ELEVATOR_CLOSED_KEYLESS
        , DOOR_ELEVATOR_CLOSED_KEYLESS
        , DOOR_ELEVATOR_OPEN_KEYED
        , DOOR_ELEVATOR_CLOSED_CALL_BUTTON });

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

}
