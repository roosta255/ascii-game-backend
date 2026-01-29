#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include "Cardinal.hpp"
#include "Character.hpp"
#include "CodeEnum.hpp"
#include "Codeset.hpp"
#include "Dungeon.hpp"
#include "Room.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "GeneratorEnum.hpp"
#include "GeneratorTutorial.hpp"
#include "RoleEnum.hpp"
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

    controller.activateCharacter(toggler1Offset);
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

    controller.activateCharacter(toggler9Offset);
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

    controller.activateCharacter(toggler3Offset);
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
