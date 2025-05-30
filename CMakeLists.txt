cmake_minimum_required(VERSION 3.10)
project(match_backend)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Drogon REQUIRED)

add_executable(match_backend
    main.cpp
    controllers/MatchApiController.cpp
    controllers/MatchController.cpp
    door/DoorFlyweight.cpp
    generator/GeneratorFlyweight.cpp
    generator/GeneratorTutorial.cpp
    generator/GeneratorUtility.cpp
    generator/iGenerator.cpp
    enum/MovementEnum.cpp
    layout/build_room_map.cpp
    layout/LayoutFlyweight.cpp
    layout/LayoutGrid.cpp
    model/Builder.cpp
    model/Cell.cpp
    model/Character.cpp
    model/Dungeon.cpp
    model/Match.cpp
    model/Player.cpp
    model/Room.cpp
    model/Titan.cpp
    model/Wall.cpp
    role/RoleFlyweight.cpp
    store/MatchStoreFile.cpp
    vector/int4_to_json.cpp
)

target_include_directories(match_backend PRIVATE
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
    ${CMAKE_CURRENT_SOURCE_DIR}/data
    ${CMAKE_CURRENT_SOURCE_DIR}/vector
)

target_link_libraries(match_backend PRIVATE Drogon::Drogon)
