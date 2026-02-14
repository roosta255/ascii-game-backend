cmake_minimum_required(VERSION 3.10)
project(match_backend)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find dependencies
find_package(Drogon REQUIRED)

# ------------ Shared include directories -------------
set(PROJECT_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/activator
    ${CMAKE_CURRENT_SOURCE_DIR}/src/algorithm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/animation
    ${CMAKE_CURRENT_SOURCE_DIR}/src/controllers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/data
    ${CMAKE_CURRENT_SOURCE_DIR}/src/door
    ${CMAKE_CURRENT_SOURCE_DIR}/src/enum
    ${CMAKE_CURRENT_SOURCE_DIR}/src/generator
    ${CMAKE_CURRENT_SOURCE_DIR}/src/inventory
    ${CMAKE_CURRENT_SOURCE_DIR}/src/layout
    ${CMAKE_CURRENT_SOURCE_DIR}/src/macro
    ${CMAKE_CURRENT_SOURCE_DIR}/src/math
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model
    ${CMAKE_CURRENT_SOURCE_DIR}/src/remodel
    ${CMAKE_CURRENT_SOURCE_DIR}/src/role
    ${CMAKE_CURRENT_SOURCE_DIR}/src/room
    ${CMAKE_CURRENT_SOURCE_DIR}/src/store
    ${CMAKE_CURRENT_SOURCE_DIR}/src/struct
    ${CMAKE_CURRENT_SOURCE_DIR}/src/time
    ${CMAKE_CURRENT_SOURCE_DIR}/src/server
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vector
    ${CMAKE_CURRENT_SOURCE_DIR}/src/view
    ${CMAKE_CURRENT_SOURCE_DIR}/src/view/api
    ${CMAKE_CURRENT_SOURCE_DIR}/src/view/store
    /usr/include/jsoncpp
)

# ------------ Shared Library -------------
add_library(core_backend STATIC
    src/activator/ActionEnum.cpp
    src/activator/ActionFlyweight.cpp
    src/activator/Activation.cpp
    src/activator/ActivatorEndTurn.cpp
    src/activator/ActivatorMoveToDoor.cpp
    src/activator/ActivatorMoveToFloor.cpp
    src/activator/ActivatorUseCharacter.cpp
    src/activator/ActivatorUseDoor.cpp
    src/activator/ActivatorUseInventoryItem.cpp
    src/activator/ActivatorUseLock.cpp
    src/activator/CharacterAction.cpp
    src/animation/AnimationEnum.cpp
    src/animation/Keyframe.cpp
    src/animation/KeyframeFlyweight.cpp
    src/controllers/MatchController.cpp
    src/door/ActivatorInactiveDoor.cpp
    src/door/ActivatorInactiveDoor.cpp
    src/door/ActivatorKeeper.cpp
    src/door/ActivatorJailer.cpp
    src/door/ActivatorLightningRod.cpp
    src/door/ActivatorShifter.cpp
    src/door/ActivatorTimeGate.cpp
    src/door/ActivatorTimeGateCube.cpp
    src/door/DoorFlyweight.cpp
    src/generator/DungeonAuthor.cpp
    src/generator/DungeonGenerator.cpp
    src/generator/DungeonMutator.cpp
    src/generator/GeneratorDoorwayDungeon1.cpp
    src/generator/GeneratorDoorwayDungeon2.cpp
    src/generator/GeneratorFlyweight.cpp
    src/generator/GeneratorPuzzle2.cpp
    src/generator/GeneratorTest.cpp
    src/generator/GeneratorTest4D.cpp
    src/generator/GeneratorTutorial.cpp
    src/generator/iGenerator.cpp
    src/enum/Codeset.cpp
    src/enum/CodeEnum.cpp
    src/enum/TurnEnum.cpp
    src/inventory/ActivatorInactiveItem.cpp
    src/inventory/Inventory.cpp
    src/inventory/Item.cpp
    src/inventory/ItemEnum.cpp
    src/inventory/ItemFlyweight.cpp
    src/layout/build_room_map.cpp
    src/layout/iLayout.cpp
    src/layout/LayoutFlyweight.cpp
    src/layout/LayoutGrid.cpp
    src/model/Account.cpp
    src/model/Builder.cpp
    src/model/Character.cpp
    src/model/Dungeon.cpp
    src/model/Match.cpp
    src/model/Player.cpp
    src/model/Titan.cpp
    src/model/Turner.cpp
    src/remodel/iRemodel.cpp
    src/remodel/Pathfinder.cpp
    src/remodel/PathfindingCounter.cpp
    src/remodel/Remodel.cpp
    src/remodel/RemodelFlyweight.cpp
    src/remodel/RemodelJailer.cpp
    src/remodel/RemodelKeeper.cpp
    src/remodel/RemodelShifter.cpp
    src/remodel/RemodelTogglerBlue.cpp
    src/remodel/RemodelTogglerOrange.cpp
    src/remodel/RemodelTogglerSwitch.cpp
    src/role/ActivatorToggler.cpp
    src/role/RoleFlyweight.cpp
    src/room/ChannelEnum.cpp
    src/room/Location.cpp
    src/room/LocationEnum.cpp
    src/room/Room.cpp
    src/room/RoomFlyweight.cpp
    src/room/Wall.cpp
    src/store/FileStore.cpp
    src/time/Timestamp.cpp
    src/vector/int4_to_json.cpp
)

# 👇 CRITICAL: Apply include dirs to the library
target_include_directories(core_backend PUBLIC ${PROJECT_INCLUDE_DIRS})

target_link_libraries(core_backend PUBLIC jsoncpp)

# ------------ Main Executable ------------
add_executable(match_backend
    main.cpp
    src/controllers/ApiController.cpp
    src/controllers/MatchRepository.cpp
    src/controllers/AccountRepository.cpp
)

target_include_directories(match_backend PRIVATE ${PROJECT_INCLUDE_DIRS})
target_link_libraries(match_backend PRIVATE
    Drogon::Drogon
    jsoncpp
    core_backend
)

# ------------ Test Target ------------
add_subdirectory(extern/Catch2)

add_executable(test_target
    test/main.cpp
    test/MatchTests.cpp
    test/TestController.cpp
    test/maps/GeneratorDoorwayDungeon1.cpp
    test/maps/GeneratorTest.cpp
    test/maps/GeneratorTest4D.cpp
    test/maps/GeneratorTutorial.cpp
)

target_include_directories(test_target PRIVATE ${PROJECT_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/test)
target_link_libraries(test_target PRIVATE
    Drogon::Drogon
    jsoncpp
    Catch2::Catch2WithMain
    core_backend
)

enable_testing()
add_test(NAME unit_tests COMMAND test_target)
