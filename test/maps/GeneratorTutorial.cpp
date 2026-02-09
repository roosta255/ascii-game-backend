#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include "Cardinal.hpp"
#include "Character.hpp"
#include "CharacterAction.hpp"
#include "CodeEnum.hpp"
#include "Codeset.hpp"
#include "Dungeon.hpp"
#include "GeneratorEnum.hpp"
#include "GeneratorTutorial.hpp"
#include "int2.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "RoleEnum.hpp"
#include "Room.hpp"
#include "TestController.hpp"

TEST_CASE("Tutorial sequence completion", "[match][tutorial]") {
    TestController controller(GENERATOR_TUTORIAL);
    int outCharacterId = -1;
    int nullCharacterId = -1;

    // Generate test layout
    controller.controller.generate(0);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());

    // Start the match
    REQUIRE(controller.match.start());

    controller.updateInventory();
    REQUIRE(controller.inventory.isEmpty);
    REQUIRE(controller.inventory.keys == 0);

    const auto findTogglerOffset = [&](int roomId, int& characterId) {
        return controller.match.findCharacter(characterId, [&](const Character& character) {
            return character.role == ROLE_TOGGLER && character.location.roomId == roomId;
        });
    };

    // Get the actual character offset of the togglers
    int toggler1Offset = -1;
    REQUIRE(findTogglerOffset(1, toggler1Offset));
    REQUIRE(toggler1Offset != -1);

    int toggler3Offset = -1;
    REQUIRE(findTogglerOffset(3, toggler3Offset));
    REQUIRE(toggler3Offset != -1);

    int toggler9Offset = -1;
    REQUIRE(findTogglerOffset(9, toggler9Offset));
    REQUIRE(toggler9Offset != -1);

    int toggler18Offset = -1;
    REQUIRE(findTogglerOffset(18, toggler18Offset));
    REQUIRE(toggler18Offset != -1);

    // Execute the tutorial sequence

    controller.moveCharacterToWall(Cardinal::east());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);
    REQUIRE(controller.builderCharacterPtr->location == Location::makeSharedDoor(1, CHANNEL_CORPOREAL, Cardinal::west()));
    REQUIRE(controller.controller.isDoorOccupied(0, CHANNEL_CORPOREAL, Cardinal::east(), outCharacterId));
    REQUIRE(controller.controller.isDoorOccupied(1, CHANNEL_CORPOREAL, Cardinal::west(), outCharacterId));
    REQUIRE(outCharacterId == controller.builderOffset);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.activateObjectCharacter(toggler1Offset);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.moveCharacterToWall(Cardinal::north());
    REQUIRE(controller.controller.isDoorOccupied(0, CHANNEL_CORPOREAL, Cardinal::east(), outCharacterId) == false);
    REQUIRE(controller.controller.isDoorOccupied(1, CHANNEL_CORPOREAL, Cardinal::west(), outCharacterId) == false);
    REQUIRE(outCharacterId == -1);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.activateObjectCharacter(toggler9Offset);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.moveCharacterToWall(Cardinal::east());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.moveCharacterToWall(Cardinal::south());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.moveCharacterToWall(Cardinal::east());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.activateObjectCharacter(toggler3Offset);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess); 

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.moveCharacterToFloor(2);
    // (controller.codeset.describe() == "");
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.moveCharacterToWall(Cardinal::west());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // Attempt to take keeper's key between rooms 2 and 3, but this should fail because character is in doorway
    REQUIRE(controller.builderCharacterPtr->location == Location::makeSharedDoor(2, CHANNEL_CORPOREAL, Cardinal::east()));
    REQUIRE(controller.controller.isDoorOccupied(2, CHANNEL_CORPOREAL, Cardinal::east(), nullCharacterId));
    controller.activateLock(Cardinal::east());
    REQUIRE(controller.codeset.getErrorTable() != Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess == false);
    REQUIRE(controller.inventory.isEmpty);
    REQUIRE(controller.inventory.keys == 0);
    controller.codeset = Codeset();

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.moveCharacterToFloor(2);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // Take keeper's key between rooms 2 and 3
    controller.activateLock(Cardinal::east());
    // REQUIRE(controller.codeset.describe() == "");
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);
    REQUIRE(controller.inventory.isEmpty == false);
    REQUIRE(controller.inventory.keys == 1);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.moveCharacterToWall(Cardinal::north());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.moveCharacterToFloor(2);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // Take key from room10's south keeper
    controller.activateLock(Cardinal::south());
    // REQUIRE(controller.codeset.describe() == "");
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);
    REQUIRE(controller.inventory.isEmpty == false);
    REQUIRE(controller.inventory.keys == 2);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // Move to room10's east wall (between room10 & room11)
    controller.moveCharacterToWall(Cardinal::east());
    // REQUIRE(controller.codeset.describe() == "");
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // Give key to room11's north shifter to open it
    controller.activateLock(Cardinal::north());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);
    REQUIRE(controller.inventory.isEmpty == false);
    REQUIRE(controller.inventory.keys == 1);

    // Move to room11's north wall (between room11 & room19)
    controller.moveCharacterToWall(Cardinal::north());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // Give key to room19's west shifter to open it
    controller.activateLock(Cardinal::west());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);
    REQUIRE(controller.inventory.isEmpty == true);
    REQUIRE(controller.inventory.keys == 0);

    // Move west to room 18
    controller.moveCharacterToWall(Cardinal::west());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.moveCharacterToFloor(2);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // TODO: Need new API to move character to floor position in room 18
    // For now this will fail as character is blocking the shifter door
    // Take key from room18's east shifter (egress side)
    controller.activateLock(Cardinal::east());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);
    REQUIRE(controller.inventory.isEmpty == false);
    REQUIRE(controller.inventory.keys == 1);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // Give key to room18's south keeper to open it
    controller.activateLock(Cardinal::south());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);
    REQUIRE(controller.inventory.isEmpty);
    REQUIRE(controller.inventory.keys == 0);

    // Move south to room 10
    controller.moveCharacterToWall(Cardinal::south());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // Move east to room 11
    controller.moveCharacterToWall(Cardinal::east());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.moveCharacterToFloor(2);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // TODO: Need new API to move character to floor position in room 11
    // For now this will fail as character is blocking the keeper door
    // Take key from room11's north keeper
    controller.activateLock(Cardinal::north());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);
    REQUIRE(controller.inventory.isEmpty == false);
    REQUIRE(controller.inventory.keys == 1);

    controller.moveCharacterToWall(Cardinal::west());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // Move north to room 18
    controller.moveCharacterToWall(Cardinal::north());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // Move character to room to allow closing of south keeper
    controller.moveCharacterToFloor(1);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // TODO: Need new API to move character to floor position in room 18
    // Take the second key from room18's south keeper
    controller.activateLock(Cardinal::south());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);
    REQUIRE(controller.inventory.isEmpty == false);
    REQUIRE(controller.inventory.keys == 2);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // Now with 2 keys, give key to room18's west keyless keeper
    controller.activateLock(Cardinal::west());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);
    REQUIRE(controller.inventory.isEmpty == false);
    REQUIRE(controller.inventory.keys == 1);

    // Move west to room 17
    controller.moveCharacterToWall(Cardinal::west());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // Give key to room17's west keyless keeper
    controller.activateLock(Cardinal::west());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);
    REQUIRE(controller.inventory.isEmpty == true);
    REQUIRE(controller.inventory.keys == 0);

    // Move west to room 16
    controller.moveCharacterToWall(Cardinal::west());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    controller.endTurn();
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    // Move north to room 24
    controller.moveCharacterToWall(Cardinal::north());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);
}

TEST_CASE("Base Case A*", "[match][tutorial]") {
    return;
    TestController controller(GENERATOR_TUTORIAL);

    // Generate test layout
    controller.controller.generate(0);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());

    // Start the match
    REQUIRE(controller.match.start());

    const auto builderOffset = controller.builderOffset;

    int moves = 0;
    const bool isFound = controller.controller.findCharacterPath(controller.BUILDER_ID, builderOffset, 5, 
        [&](const Match& match){
            Match copyMatch = match;
            bool isSuccess = false;
            copyMatch.getCharacter(builderOffset, controller.codeset.error).access([&](Character& character){
                isSuccess = character.location.roomId == 0;
            });
            return isSuccess;
        }, [&](const Match& match){
            return 0; // no heuristic
        },
        [&](const CharacterAction& action, const Match& match){
            moves++;
        });
    REQUIRE(isFound);
    REQUIRE(moves == 0);
}

TEST_CASE("Check A* moves east", "[match][tutorial]") {
    TestController controller(GENERATOR_TUTORIAL);

    // Generate test layout
    controller.controller.generate(0);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());

    // Start the match
    REQUIRE(controller.match.start());

    const auto builderOffset = controller.builderOffset;

    int moves = 0;
    Array<CharacterAction, 1> actualActions;
    const bool isFound = controller.controller.findCharacterPath(controller.BUILDER_ID, builderOffset, 5, 
        [&](const Match& match){
            Match copyMatch = match;
            bool isSuccess = false;
            copyMatch.getCharacter(builderOffset, controller.codeset.error).access([&](Character& character){
                isSuccess = character.location.roomId == 1;
            });
            return isSuccess;
        }, [&](const Match& match){
            return 0; // no heuristic
        },
        [&](const CharacterAction& action, const Match& match){
            actualActions.set(moves++, action);
        });
    REQUIRE(isFound);
    REQUIRE(actualActions == std::array<CharacterAction, 1>{CharacterAction{.type = ACTION_MOVE_TO_DOOR, .characterId = builderOffset, .roomId = 0, .direction = Cardinal::east()}});
}

int expectedPathfindingLoops(int steps) {
    constexpr static auto PATHFINDING_LOOP_FACTOR = 6;
    constexpr static auto PATHFINDING_LOOP_OFFSET = 6;
    return steps * PATHFINDING_LOOP_FACTOR + PATHFINDING_LOOP_OFFSET;
}

TEST_CASE("Check A* moves east then runs toggler", "[match][tutorial]") {
    TestController controller(GENERATOR_TUTORIAL);

    // Generate test layout
    controller.controller.generate(0);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());

    // Start the match
    REQUIRE(controller.match.start());

    const auto builderOffset = controller.builderOffset;
    controller.codeset.setTable(CODE_BUILDER_CHARACTER_ID, builderOffset);
    controller.codeset.setTable(CODE_BUILDER_ROLE, ROLE_BUILDER);

    const auto findTogglerOffset = [&](int roomId, int& characterId) {
        return controller.match.findCharacter(characterId, [&](const Character& character) {
            return character.role == ROLE_TOGGLER && character.location.roomId == roomId;
        });
    };

    // Get the actual character offset of the togglers
    int toggler1Offset = -1;
    REQUIRE(findTogglerOffset(1, toggler1Offset));
    REQUIRE(toggler1Offset != -1);
    controller.codeset.setTable(CODE_TOGGLER_CHARACTER_ID, toggler1Offset);
    controller.codeset.setTable(CODE_TOGGLER_ROLE, ROLE_TOGGLER);

    int toggler3Offset = -1;
    REQUIRE(findTogglerOffset(3, toggler3Offset));
    REQUIRE(toggler3Offset != -1);

    int toggler9Offset = -1;
    REQUIRE(findTogglerOffset(9, toggler9Offset));
    REQUIRE(toggler9Offset != -1);

    int toggler18Offset = -1;
    REQUIRE(findTogglerOffset(18, toggler18Offset));
    REQUIRE(toggler18Offset != -1);

    int moves = 0;
    constexpr static auto EXPECTED_ACTIONS = 6;
    Array<CharacterAction, EXPECTED_ACTIONS> actualActions;
    const bool isFound = controller.controller.findCharacterPath(controller.BUILDER_ID, builderOffset, expectedPathfindingLoops(EXPECTED_ACTIONS), 
        [&](const Match& match){
            return match.dungeon.isBlueOpen;
        }, [&](const Match& match){
            return 0; // no heuristic
        },
        [&](const CharacterAction& action, const Match& match){
            actualActions.set(moves++, action);
        });
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(isFound);
    REQUIRE(actualActions == std::array<CharacterAction, EXPECTED_ACTIONS>
        { CharacterAction{.type = ACTION_MOVE_TO_DOOR, .characterId = builderOffset, .roomId = 0, .direction = Cardinal::east()}
        , CharacterAction{.type = ACTION_ACTIVATE_CHARACTER, .characterId = toggler1Offset, .roomId = 1, .targetCharacterId = builderOffset}});
}

TEST_CASE("Check A* moves east then runs toggler, then goes north, then runs toggler to move east", "[match][tutorial]") {
    TestController controller(GENERATOR_TUTORIAL);

    // Generate test layout
    controller.controller.generate(0);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());

    // Start the match
    REQUIRE(controller.match.start());

    const auto builderOffset = controller.builderOffset;
    controller.codeset.setTable(CODE_BUILDER_CHARACTER_ID, builderOffset);
    controller.codeset.setTable(CODE_BUILDER_ROLE, ROLE_BUILDER);

    const auto findTogglerOffset = [&](int roomId, int& characterId) {
        return controller.match.findCharacter(characterId, [&](const Character& character) {
            return character.role == ROLE_TOGGLER && character.location.roomId == roomId;
        });
    };

    // Get the actual character offset of the togglers
    int toggler1Offset = -1;
    REQUIRE(findTogglerOffset(1, toggler1Offset));
    REQUIRE(toggler1Offset != -1);
    controller.codeset.setTable(CODE_TOGGLER_CHARACTER_ID, toggler1Offset);
    controller.codeset.setTable(CODE_TOGGLER_ROLE, ROLE_TOGGLER);

    int toggler3Offset = -1;
    REQUIRE(findTogglerOffset(3, toggler3Offset));
    REQUIRE(toggler3Offset != -1);

    int toggler9Offset = -1;
    REQUIRE(findTogglerOffset(9, toggler9Offset));
    REQUIRE(toggler9Offset != -1);

    int toggler18Offset = -1;
    REQUIRE(findTogglerOffset(18, toggler18Offset));
    REQUIRE(toggler18Offset != -1);

    int moves = 0;
    constexpr static auto EXPECTED_ACTIONS = 5;
    Array<CharacterAction, EXPECTED_ACTIONS> actualActions;
    const bool isFound = controller.controller.findCharacterPath(controller.BUILDER_ID, builderOffset, expectedPathfindingLoops(EXPECTED_ACTIONS), 
        [&](const Match& match){
            Match copyMatch = match;
            bool isSuccess = false;
            copyMatch.getCharacter(builderOffset, controller.codeset.error).access([&](Character& character){
                isSuccess = character.location.roomId == 10;
            });
            return isSuccess;
        }, [&](const Match& match){
            return 0; // no heuristic
        },
        [&](const CharacterAction& action, const Match& match){
            actualActions.set(moves++, action);
        });
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(isFound);
    REQUIRE(moves == EXPECTED_ACTIONS);
    REQUIRE(actualActions == std::array<CharacterAction, EXPECTED_ACTIONS>
        { CharacterAction{.type = ACTION_MOVE_TO_DOOR, .characterId = builderOffset, .roomId = 0, .direction = Cardinal::east()}
        , CharacterAction{.type = ACTION_ACTIVATE_CHARACTER, .characterId = toggler1Offset, .roomId = 1, .targetCharacterId = builderOffset}
        , CharacterAction{.type = ACTION_MOVE_TO_DOOR, .characterId = builderOffset, .roomId = 1, .direction = Cardinal::north()}
        , CharacterAction{.type = ACTION_ACTIVATE_CHARACTER, .characterId = toggler9Offset, .roomId = 9, .targetCharacterId = builderOffset}
        , CharacterAction{.type = ACTION_MOVE_TO_DOOR, .characterId = builderOffset, .roomId = 9, .direction = Cardinal::east()}
        });
}

TEST_CASE("Check A* can grab key from keeper", "[match][tutorial]") {
    TestController controller(GENERATOR_TUTORIAL);

    // Generate test layout
    controller.controller.generate(0);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());

    // Start the match
    REQUIRE(controller.match.start());

    const auto builderOffset = controller.builderOffset;
    controller.codeset.setTable(CODE_BUILDER_CHARACTER_ID, builderOffset);
    controller.codeset.setTable(CODE_BUILDER_ROLE, ROLE_BUILDER);

    const auto findTogglerOffset = [&](int roomId, int& characterId) {
        return controller.match.findCharacter(characterId, [&](const Character& character) {
            return character.role == ROLE_TOGGLER && character.location.roomId == roomId;
        });
    };

    controller.match.dungeon.getRoom(10, controller.codeset.error).accessConst([&](const Room& room){
        controller.codeset.setTable(CODE_PATHFINDING_EXPECTED_LOCK_TYPE, room.getWall(Cardinal::south()).door);
    });

    // Get the actual character offset of the togglers
    int toggler1Offset = -1;
    REQUIRE(findTogglerOffset(1, toggler1Offset));
    REQUIRE(toggler1Offset != -1);
    controller.codeset.setTable(CODE_TOGGLER_CHARACTER_ID, toggler1Offset);
    controller.codeset.setTable(CODE_TOGGLER_ROLE, ROLE_TOGGLER);

    int toggler3Offset = -1;
    REQUIRE(findTogglerOffset(3, toggler3Offset));
    REQUIRE(toggler3Offset != -1);

    int toggler9Offset = -1;
    REQUIRE(findTogglerOffset(9, toggler9Offset));
    REQUIRE(toggler9Offset != -1);

    int toggler18Offset = -1;
    REQUIRE(findTogglerOffset(18, toggler18Offset));
    REQUIRE(toggler18Offset != -1);

    int moves = 0;
    constexpr static auto EXPECTED_ACTIONS = 6;
    Array<CharacterAction, EXPECTED_ACTIONS> actualActions;
    const bool isFound = controller.controller.findCharacterPath(controller.BUILDER_ID, builderOffset, expectedPathfindingLoops(EXPECTED_ACTIONS), 
        [&](const Match& match){
            Match copyMatch = match;
            bool isSuccess = false;
            match.builders.getPointer(0).accessConst([&](const Builder& builder){
                isSuccess = builder.character.location.roomId == 10 && builder.player.inventory.makeDigest().keys > 0;
            });
            return isSuccess;
        }, [&](const Match& match){
            return 0; // no heuristic
        },
        [&](const CharacterAction& action, const Match& match){
            actualActions.set(moves++, action);
        });
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    // REQUIRE(controller.codeset.describe() == "");
    REQUIRE(isFound);
    REQUIRE(moves == EXPECTED_ACTIONS);
    REQUIRE(actualActions == std::array<CharacterAction, EXPECTED_ACTIONS>
        { CharacterAction{.type=ACTION_MOVE_TO_DOOR, .characterId=builderOffset, .roomId = 0, .direction = Cardinal::east()}
        , CharacterAction{.type=ACTION_ACTIVATE_CHARACTER, .characterId=toggler1Offset, .roomId = 1, .targetCharacterId = builderOffset}
        , CharacterAction{.type=ACTION_MOVE_TO_DOOR, .characterId=builderOffset, .roomId = 1, .direction = Cardinal::north()}
        , CharacterAction{.type=ACTION_ACTIVATE_CHARACTER, .characterId=toggler9Offset, .roomId=9, .targetCharacterId=builderOffset }
        , CharacterAction{.type=ACTION_MOVE_TO_DOOR, .characterId=builderOffset, .roomId=9, .direction=Cardinal::east() }
        , CharacterAction{.type=ACTION_ACTIVATE_LOCK, .characterId=builderOffset, .roomId=10, .direction=Cardinal::south() }
        });
}

TEST_CASE("Check A* can complete match", "[match][tutorial]") {
    TestController controller(GENERATOR_TUTORIAL);

    // Generate test layout
    controller.controller.generate(0);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());

    // Start the match
    REQUIRE(controller.match.start());

    const auto builderOffset = controller.builderOffset;
    controller.codeset.setTable(CODE_BUILDER_CHARACTER_ID, builderOffset);
    controller.codeset.setTable(CODE_BUILDER_ROLE, ROLE_BUILDER);

    const auto findTogglerOffset = [&](int roomId, int& characterId) {
        return controller.match.findCharacter(characterId, [&](const Character& character) {
            return character.role == ROLE_TOGGLER && character.location.roomId == roomId;
        });
    };

    controller.match.dungeon.getRoom(10, controller.codeset.error).accessConst([&](const Room& room){
        controller.codeset.setTable(CODE_PATHFINDING_EXPECTED_LOCK_TYPE, room.getWall(Cardinal::south()).door);
    });

    // Get the actual character offset of the togglers
    int toggler1Offset = -1;
    REQUIRE(findTogglerOffset(1, toggler1Offset));
    REQUIRE(toggler1Offset != -1);
    controller.codeset.setTable(CODE_TOGGLER_CHARACTER_ID, toggler1Offset);
    controller.codeset.setTable(CODE_TOGGLER_ROLE, ROLE_TOGGLER);

    int toggler3Offset = -1;
    REQUIRE(findTogglerOffset(3, toggler3Offset));
    REQUIRE(toggler3Offset != -1);

    int toggler9Offset = -1;
    REQUIRE(findTogglerOffset(9, toggler9Offset));
    REQUIRE(toggler9Offset != -1);

    int toggler18Offset = -1;
    REQUIRE(findTogglerOffset(18, toggler18Offset));
    REQUIRE(toggler18Offset != -1);

    int moves = 0;
    constexpr static auto EXPECTED_ACTIONS = 27;
    Array<CharacterAction, EXPECTED_ACTIONS> actualActions;
    const bool isFound = controller.controller.findCharacterPath(controller.BUILDER_ID, builderOffset, 400, 
        [&](const Match& match){
            Match copyMatch = match;
            bool isSuccess = false;
            match.builders.getPointer(0).accessConst([&](const Builder& builder){
                isSuccess = builder.character.location.roomId == 24;
            });
            return isSuccess;
        }, [&](const Match& match){
            return 0; // no heuristic
        },
        [&](const CharacterAction& action, const Match& match){
            actualActions.set(moves++, action);
        });
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    // REQUIRE(controller.codeset.describe() == "");
    REQUIRE(isFound);
    REQUIRE(moves == EXPECTED_ACTIONS);
    REQUIRE(actualActions == std::array<CharacterAction, EXPECTED_ACTIONS>
        { CharacterAction{.type=ACTION_MOVE_TO_DOOR, .characterId=builderOffset, .roomId=0, .direction=Cardinal::east() }
        , CharacterAction{.type=ACTION_ACTIVATE_CHARACTER, .characterId=toggler1Offset, .roomId=1, .targetCharacterId=builderOffset }
        , CharacterAction{.type=ACTION_MOVE_TO_DOOR, .characterId=builderOffset, .roomId=1, .direction=Cardinal::north() }
        , CharacterAction{.type=ACTION_ACTIVATE_CHARACTER, .characterId=toggler9Offset, .roomId=9, .targetCharacterId=builderOffset }
        , CharacterAction{.type=ACTION_MOVE_TO_DOOR, .characterId=builderOffset, .roomId=9, .direction=Cardinal::east() }
        , CharacterAction{.type=ACTION_ACTIVATE_CHARACTER, .characterId=toggler9Offset, .roomId=9, .targetCharacterId=builderOffset }
        , CharacterAction{.type=ACTION_MOVE_TO_DOOR, .characterId=builderOffset, .roomId=10, .direction=Cardinal::south() }
        , CharacterAction{.type=ACTION_ACTIVATE_LOCK, .characterId=builderOffset, .roomId=2, .direction=Cardinal::east() }
        , CharacterAction{.type=ACTION_MOVE_TO_DOOR, .characterId=builderOffset, .roomId=10, .direction=Cardinal::east() }
        , CharacterAction{.type=ACTION_ACTIVATE_LOCK, .characterId=builderOffset, .roomId=10, .direction=Cardinal::south() }
        , CharacterAction{.type=ACTION_ACTIVATE_LOCK, .characterId=builderOffset, .roomId=11, .direction=Cardinal::north() }
        , CharacterAction{.type=ACTION_MOVE_TO_DOOR, .characterId=builderOffset, .roomId=11, .direction=Cardinal::north() }
        , CharacterAction{.type=ACTION_ACTIVATE_LOCK, .characterId=builderOffset, .roomId=19, .direction=Cardinal::west() }
        , CharacterAction{.type=ACTION_MOVE_TO_DOOR, .characterId=builderOffset, .roomId=19, .direction=Cardinal::west() }
        , CharacterAction{.type=ACTION_MOVE_TO_FLOOR, .characterId=builderOffset, .roomId=18, .floorId=1 }
        , CharacterAction{.type=ACTION_ACTIVATE_LOCK, .characterId=builderOffset, .roomId=18, .direction=Cardinal::east() }
        , CharacterAction{.type=ACTION_ACTIVATE_LOCK, .characterId=builderOffset, .roomId=18, .direction=Cardinal::south() }
        , CharacterAction{.type=ACTION_MOVE_TO_DOOR, .characterId=builderOffset, .roomId=18, .direction=Cardinal::south() }
        , CharacterAction{.type=ACTION_MOVE_TO_DOOR, .characterId=builderOffset, .roomId=10, .direction=Cardinal::east() }
        , CharacterAction{.type=ACTION_ACTIVATE_LOCK, .characterId=builderOffset, .roomId=11, .direction=Cardinal::north() }
        , CharacterAction{.type=ACTION_MOVE_TO_DOOR, .characterId=builderOffset, .roomId=10, .direction=Cardinal::north() }
        , CharacterAction{.type=ACTION_ACTIVATE_LOCK, .characterId=builderOffset, .roomId=18, .direction=Cardinal::west() }
        , CharacterAction{.type=ACTION_MOVE_TO_DOOR, .characterId=builderOffset, .roomId=18, .direction=Cardinal::west() }
        , CharacterAction{.type=ACTION_ACTIVATE_LOCK, .characterId=builderOffset, .roomId=18, .direction=Cardinal::south() }
        , CharacterAction{.type=ACTION_ACTIVATE_LOCK, .characterId=builderOffset, .roomId=17, .direction=Cardinal::west() }
        , CharacterAction{.type=ACTION_MOVE_TO_DOOR, .characterId=builderOffset, .roomId=17, .direction=Cardinal::west() }
        , CharacterAction{.type=ACTION_MOVE_TO_DOOR, .characterId=builderOffset, .roomId=16, .direction=Cardinal::north() }
        });
}

void processMatchCollapse(TestController& controller, std::function<void()> actions, std::function<void(const Match&, const Match&)> comparison){
    auto before = controller.match;
    before.setPathfinding();
    actions();
    auto after = controller.match;
    after.setPathfinding();
    comparison(before, after);
}

TEST_CASE("Match collapsing base case", "[match][tutorial]") {
    TestController controller(GENERATOR_TUTORIAL);
    controller.isSkippingAnimations = true;

    // Generate test layout
    controller.controller.generate(0);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());

    // Start the match
    REQUIRE(controller.match.start());

    processMatchCollapse(controller, [&](){}, [&](const Match& before, const Match& after){ REQUIRE(before == after); });
}

TEST_CASE("Match collapsing end turn", "[match][tutorial]") {
    TestController controller(GENERATOR_TUTORIAL);
    controller.isSkippingAnimations = true;

    // Generate test layout
    controller.controller.generate(0);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());

    // Start the match
    REQUIRE(controller.match.start());

    processMatchCollapse(controller, [&](){
            controller.endTurn();
            REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
            REQUIRE(controller.isSuccess);
        }, [&](const Match& before, const Match& after){ REQUIRE(before == after); });
}

TEST_CASE("Match collapsing from floor move to wall", "[match][tutorial]") {
    TestController controller(GENERATOR_TUTORIAL);
    controller.isSkippingAnimations = true;

    // Generate test layout
    controller.controller.generate(0);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());

    // Start the match
    REQUIRE(controller.match.start());

    const auto ROOM_ID = 0;
    const auto FLOOR_ID = 3;
    controller.moveCharacterToFloor(ROOM_ID, FLOOR_ID);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    processMatchCollapse(controller, [&](){
            controller.moveCharacterToWall(ROOM_ID, Cardinal::east());
            REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
            REQUIRE(controller.isSuccess);

            controller.endTurn();
            REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
            REQUIRE(controller.isSuccess);

            // this is not the correct room
            controller.moveCharacterToFloor(ROOM_ID, FLOOR_ID);
            REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
            REQUIRE(controller.isSuccess);
        }, [&](const Match& before, const Match& after){
            REQUIRE(before.dungeon == after.dungeon);
            REQUIRE(before == after);
        });
}

TEST_CASE("Match collapsing from wall move to floor", "[match][tutorial]") {
    TestController controller(GENERATOR_TUTORIAL);
    controller.isSkippingAnimations = true;

    // Generate test layout
    controller.controller.generate(0);
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());

    // Start the match
    REQUIRE(controller.match.start());

    const auto ROOM_ID = 0;
    controller.moveCharacterToWall(ROOM_ID, Cardinal::east());
    REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
    REQUIRE(controller.isSuccess);

    const auto FLOOR_ID = 3;
    processMatchCollapse(controller, [&](){
            // this is not room 0, it's room 1
            controller.moveCharacterToFloor(ROOM_ID, FLOOR_ID);
            REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
            REQUIRE(controller.isSuccess);

            controller.endTurn();
            REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
            REQUIRE(controller.isSuccess);

            controller.moveCharacterToWall(ROOM_ID, Cardinal::east());
            REQUIRE(controller.codeset.getErrorTable() == Codeset::getEmptyTable());
            REQUIRE(controller.isSuccess);
        }, [&](const Match& before, const Match& after){
            REQUIRE(before.dungeon == after.dungeon);
            REQUIRE(before == after);
        });
}

// TODO: sort character location roomIds
// TODO: sort characters in floorIds
// TODO: sort items in inventory
