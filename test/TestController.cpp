
#include "Array.hpp"
#include "Builder.hpp"
#include "Cardinal.hpp"
#include "Dungeon.hpp"
#include "Inventory.hpp"
#include "InventoryDigest.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Room.hpp"
#include "TestController.hpp"

// constructors
TestController::TestController(const GeneratorEnum& generator): controller(match, codeset) {
    match.host = BUILDER_ID;
    match.filename = MATCH_ID;
    match.generator = generator;
    match.builders.access(BUILDER_INDEX, [&](Builder& builder) {
        match.containsCharacter(builder.character, builderOffset);
        inventoryPtr = &builder.player.inventory;
        builderCharacterPtr = &builder.character;
    });
}

// functions
void TestController::activateCharacter(int characterId){
    isSuccess = controller.activateCharacter(BUILDER_ID, characterId, latestPosition, builderOffset);
    updateEverything();
}

void TestController::activateDoor(Cardinal dir){
    isSuccess = controller.activateDoor(BUILDER_ID, builderOffset, latestPosition, dir.getIndex());
    updateEverything();
}

void TestController::activateLock(Cardinal dir){
    isSuccess = controller.activateLock(BUILDER_ID, builderOffset, latestPosition, dir.getIndex());
    updateEverything();
}

void TestController::endTurn(){
    isSuccess = codeset.addSuccessElseFailure(match.endTurn(BUILDER_ID, codeset.error));
    updateEverything();
}

void TestController::moveCharacterToFloor(int floorId) {
    isSuccess = controller.moveCharacterToFloor(latestPosition, builderOffset, floorId);
    updateEverything();
}

void TestController::moveCharacterToWall(const Cardinal& dir){
    isSuccess = controller.moveCharacterToWall(latestPosition, builderOffset, dir);
    // TODO: add test codes for this line
    match.dungeon.rooms.accessConst(latestPosition, [&](const Room& room){
        if (isSuccess) {
            latestPosition = room.getWall(dir).adjacent;
        }
        // TODO: add test codes for this line
        match.dungeon.rooms.containsIndex(latestPosition);
    });
    updateEverything();
}

void TestController::updateEverything() {
    updateInventory();
    updateTable();
}

void TestController::updateInventory(){
    inventory = inventoryPtr->makeDigest();
}

void TestController::updateTable() {
    codeset.setTable(CODE_TEST_BUILDER_LATEST_EXPECTED_ROOM_ID, latestPosition);
    match.builders.access(BUILDER_INDEX, [&](Builder& builder) {
        codeset.setTable(CODE_TEST_BUILDER_LATEST_ACTUAL_ROOM_ID, builder.character.location.roomId);
    });
}

