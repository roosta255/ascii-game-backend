
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
    });
}

// functions
void TestController::updateInventory(){
    inventory = inventoryPtr->makeDigest();
}

void TestController::moveCharacterToWall(const Cardinal& dir){
    isSuccess = controller.moveCharacterToWall(latestPosition, builderOffset, dir);
    // TODO: add test codes for this line
    match.dungeon.rooms.accessConst(latestPosition, [&](const Room& room){
        latestPosition = room.getWall(dir).adjacent;
        // TODO: add test codes for this line
        match.dungeon.rooms.containsIndex(latestPosition);
    });
    updateInventory();
}

void TestController::moveCharacterToFloor(int floorId) {
    isSuccess = controller.moveCharacterToFloor(latestPosition, builderOffset, floorId);
    updateInventory();
}

void TestController::activateLock(Cardinal dir){
    isSuccess = controller.activateLock(BUILDER_ID, builderOffset, latestPosition, dir.getIndex());
    updateInventory();
}

void TestController::activateDoor(Cardinal dir){
    isSuccess = controller.activateDoor(BUILDER_ID, builderOffset, latestPosition, dir.getIndex());
    updateInventory();
}

void TestController::endTurn(){
    isSuccess = codeset.addSuccessElseFailure(match.endTurn(BUILDER_ID, codeset.error));
    updateInventory();
}
