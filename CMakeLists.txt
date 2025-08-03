cmake_minimum_required(VERSION 3.10)
project(match_backend)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find dependencies
find_package(Drogon REQUIRED)

# ------------ Shared include directories -------------
set(PROJECT_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/algorithm
    ${CMAKE_CURRENT_SOURCE_DIR}/controllers
    ${CMAKE_CURRENT_SOURCE_DIR}/door
    ${CMAKE_CURRENT_SOURCE_DIR}/enum
    ${CMAKE_CURRENT_SOURCE_DIR}/generator
    ${CMAKE_CURRENT_SOURCE_DIR}/layout
    ${CMAKE_CURRENT_SOURCE_DIR}/math
    ${CMAKE_CURRENT_SOURCE_DIR}/model
    ${CMAKE_CURRENT_SOURCE_DIR}/role
    ${CMAKE_CURRENT_SOURCE_DIR}/store
    ${CMAKE_CURRENT_SOURCE_DIR}/struct
    ${CMAKE_CURRENT_SOURCE_DIR}/data
    ${CMAKE_CURRENT_SOURCE_DIR}/server
    ${CMAKE_CURRENT_SOURCE_DIR}/vector
    ${CMAKE_CURRENT_SOURCE_DIR}/view
    ${CMAKE_CURRENT_SOURCE_DIR}/view/api
    ${CMAKE_CURRENT_SOURCE_DIR}/view/store
    /usr/include/jsoncpp
)

# ------------ Shared Library -------------
add_library(core_backend STATIC
    door/ActivatorInactiveDoor.cpp
    door/ActivatorKeeper.cpp
    door/ActivatorShifter.cpp
    door/DoorFlyweight.cpp
    generator/GeneratorFlyweight.cpp
    generator/GeneratorPuzzle2.cpp
    generator/GeneratorTutorial.cpp
    generator/GeneratorUtility.cpp
    generator/iGenerator.cpp
    enum/CodeEnum.cpp
    enum/TurnEnum.cpp
    layout/build_room_map.cpp
    layout/iLayout.cpp
    layout/LayoutFlyweight.cpp
    layout/LayoutGrid.cpp
    model/Account.cpp
    model/Builder.cpp
    model/Cell.cpp
    model/Character.cpp
    model/Dungeon.cpp
    model/Match.cpp 
    model/Player.cpp
    model/Room.cpp
    model/Titan.cpp
    model/Turner.cpp
    model/Wall.cpp
    role/ActivatorToggler.cpp
    role/RoleFlyweight.cpp
    store/FileStore.cpp
    vector/int4_to_json.cpp
)

# 👇 CRITICAL: Apply include dirs to the library
target_include_directories(core_backend PUBLIC ${PROJECT_INCLUDE_DIRS})

target_link_libraries(core_backend PUBLIC jsoncpp)

# ------------ Main Executable ------------
add_executable(match_backend
    main.cpp
    controllers/ApiController.cpp
    controllers/MatchController.cpp
    controllers/AccountController.cpp
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
