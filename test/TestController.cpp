
#include "ActionFlyweight.hpp"
#include "Array.hpp"
#include "Builder.hpp"
#include "Cardinal.hpp"
#include "Dungeon.hpp"
#include "Inventory.hpp"
#include "InventoryDigest.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Preactivation.hpp"
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
void TestController::activateCharacter(int subjectCharacterId, int objectCharacterId){
    Preactivation preactivation{
        .playerId = BUILDER_ID,
        .characterId = subjectCharacterId,
        .roomId = latestPosition,
        .targetCharacterId = objectCharacterId,
        .isSkippingAnimations = isSkippingAnimations
    };

    isSuccess = controller.activate(ActionFlyweight::getUseCharacter(), preactivation);
    updateEverything();
}

void TestController::activateDoor(Cardinal dir){
    Preactivation preactivation{
        .playerId = BUILDER_ID,
        .characterId = builderOffset,
        .roomId = latestPosition,
        .direction = dir,
        .isSkippingAnimations = isSkippingAnimations
    };

    isSuccess = controller.activate(ActionFlyweight::getUseDoor(), preactivation);
    updateEverything();
}

void TestController::activateLock(Cardinal dir){
    Preactivation preactivation{
        .playerId = BUILDER_ID,
        .characterId = builderOffset,
        .roomId = latestPosition,
        .direction = dir,
        .isSkippingAnimations = isSkippingAnimations
    };
    isSuccess = controller.activate(ActionFlyweight::getUseLock(), preactivation);
    updateEverything();
}

void TestController::activateObjectCharacter(int objectCharacterId){
    Preactivation preactivation{
        .playerId = BUILDER_ID,
        .characterId = objectCharacterId,
        .roomId = latestPosition,
        .targetCharacterId = builderOffset,
        .isSkippingAnimations = isSkippingAnimations
    };
    isSuccess = controller.activate(ActionFlyweight::getUseCharacter(), preactivation);
    updateEverything();
}

Remodel TestController::buildRemodel(int bossRoomId, std::function<bool(const Match&, const PathfindingCounter&)> acceptance) {
    return Remodel {
        .playerId= BUILDER_ID,
        .characterId= builderOffset,
        .bossRoomId= bossRoomId,
        .codeset= codeset,
        .acceptance= acceptance
    };
}

void TestController::endTurn(){
    Preactivation preactivation{
        .playerId = BUILDER_ID,
        .characterId = builderOffset,
        .roomId = latestPosition,
        .isSkippingAnimations = isSkippingAnimations
    };
    codeset.addFailure(!(isSuccess = match.endTurn(BUILDER_ID, codeset.error)));
    updateEverything();
}

void TestController::moveCharacterToFloor(int roomId, int floorId) {
    Preactivation preactivation{
        .playerId = BUILDER_ID,
        .characterId = builderOffset,
        .roomId = roomId,
        .floorId = floorId,
        .isSkippingAnimations = isSkippingAnimations
    };
    isSuccess = controller.activate(ActionFlyweight::getMoveToFloor(), preactivation);
    updateEverything();
}

void TestController::moveCharacterToFloor(int floorId) {
    moveCharacterToFloor(latestPosition, floorId);
}

void TestController::moveCharacterToWall(int roomId, const Cardinal& dir){
    Preactivation preactivation{
        .playerId = BUILDER_ID,
        .characterId = builderOffset,
        .roomId = roomId,
        .direction = dir,
        .isSkippingAnimations = isSkippingAnimations
    };
    isSuccess = controller.activate(ActionFlyweight::getMoveToDoor(), preactivation);
    // TODO: add test codes for this line
    match.dungeon.rooms.accessConst(roomId, [&](const Room& room){
        if (isSuccess) {
            latestPosition = room.getWall(dir).adjacent;
        }
        // TODO: add test codes for this line
        match.dungeon.rooms.containsIndex(latestPosition);
    });
    updateEverything();
}

void TestController::moveCharacterToWall(const Cardinal& dir){
    moveCharacterToWall(latestPosition, dir);
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

