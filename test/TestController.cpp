
#include "ActionFlyweight.hpp"
#include "Array.hpp"
#include "Builder.hpp"
#include "Cardinal.hpp"
#include "Dungeon.hpp"
#include "Inventory.hpp"
#include "InventoryDigest.hpp"
#include "Match.hpp"
#include "MatchApiParameters.hpp"
#include "MatchApiView.hpp"
#include "MatchController.hpp"
#include "MatchStoreView.hpp"
#include "Preactivation.hpp"
#include "Room.hpp"
#include "TestController.hpp"
#include <nlohmann/json.hpp>

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
        .action = {
            .type = ACTION_ACTIVATE_CHARACTER,
            .characterId = subjectCharacterId,
            .roomId = latestPosition,
            .targetCharacterId = objectCharacterId,
        },
        .playerId = BUILDER_ID,
        .isSkippingAnimations = isSkippingAnimations
    };

    isSuccess = controller.activate(preactivation);
    updateEverything();
}

void TestController::activateDoor(Cardinal dir){
    Preactivation preactivation{
        .action = {
            .type = ACTION_ACTIVATE_DOOR,
            .characterId = builderOffset,
            .roomId = latestPosition,
            .direction = dir,
        },
        .playerId = BUILDER_ID,
        .isSkippingAnimations = isSkippingAnimations
    };

    isSuccess = controller.activate(preactivation);
    updateEverything();
}

void TestController::activateLock(Cardinal dir){
    Preactivation preactivation{
        .action = {
            .type = ACTION_ACTIVATE_LOCK,
            .characterId = builderOffset,
            .roomId = latestPosition,
            .direction = dir,
        },
        .playerId = BUILDER_ID,
        .isSkippingAnimations = isSkippingAnimations
    };
    isSuccess = controller.activate(preactivation);
    updateEverything();
}

void TestController::activateObjectCharacter(int objectCharacterId){
    Preactivation preactivation{
        .action = {
            .type = ACTION_ACTIVATE_CHARACTER,
            .characterId = objectCharacterId,
            .roomId = latestPosition,
            .targetCharacterId = builderOffset,
        },
        .playerId = BUILDER_ID,
        .isSkippingAnimations = isSkippingAnimations
    };
    isSuccess = controller.activate(preactivation);
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
    codeset.addFailure(!(isSuccess = match.endTurn(BUILDER_ID, codeset.error)));
    updateEverything();
}

void TestController::generate(int seed) {
    isSuccess = controller.generate(seed);
    latestPosition = builderCharacterPtr->location.roomId;
}

void TestController::giveItem(ItemEnum type) {
    isSuccess = inventoryPtr->giveItem(type, codeset.error);
    updateInventory();
}

void TestController::lootInventory(int containerCharacterId, const ItemEnum& targetItemType) {
    Preactivation preactivation{
        .action = {
            .type = ACTION_LOOT_CHEST,
            .characterId = builderOffset,
            .roomId = latestPosition,
            .targetCharacterId = containerCharacterId,
        },
        .playerId = BUILDER_ID,
        .isSkippingAnimations = isSkippingAnimations
    };
    match.dungeon.findChestByContainerId(containerCharacterId, codeset.error).access([&](Chest& chest) {
        int targetInventoryId = -1;
        if (match.containsInventory(chest.inventory, targetInventoryId)) {
            preactivation.action.targetInventoryIndex = targetInventoryId;
        }
        chest.inventory.accessItem(targetItemType, [&](const Item& item){
            // item.type is correctly the ITEM_KEY_ELEVATOR
            int targetItemIndex = -1;
            if (chest.inventory.items.containsAddress(item, targetItemIndex)) {
                preactivation.action.targetItemIndex = targetItemIndex;
            }
        });
    });
    isSuccess = controller.activate(preactivation);
    updateInventory();
}

void TestController::moveCharacterToFloor(int roomId, int floorId) {
    Preactivation preactivation{
        .action = {
            .type = ACTION_MOVE_TO_FLOOR,
            .characterId = builderOffset,
            .roomId = roomId,
            .floorId = floorId,
        },
        .playerId = BUILDER_ID,
        .isSkippingAnimations = isSkippingAnimations
    };
    isSuccess = controller.activate(preactivation);
    updateEverything();
}

void TestController::moveCharacterToFloor(int floorId) {
    moveCharacterToFloor(latestPosition, floorId);
}

void TestController::moveCharacterToWall(int roomId, const Cardinal& dir){
    Preactivation preactivation{
        .action = {
            .type = ACTION_MOVE_TO_DOOR,
            .characterId = builderOffset,
            .roomId = roomId,
            .direction = dir,
        },
        .playerId = BUILDER_ID,
        .isSkippingAnimations = isSkippingAnimations
    };
    isSuccess = controller.activate(preactivation);
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

Match TestController::saveAndLoadMatch() {
    nlohmann::json j = MatchStoreView(match);
    Match loaded = (Match)j.get<MatchStoreView>();
    loaded.accessAllInventories([&](Inventory& inventory) {
        loaded.containsInventory(inventory, inventory.inventoryId);
    });
    return loaded;
}

MatchApiView TestController::getMatchApiView() {
    MatchApiParameters params{
        .mask = ~0x0,
        .match = match,
        .doors = controller.getDoors(),
        .floors = controller.getFloors(),
        .traitsComputed = controller.getTraitsComputedMap()
    };
    return MatchApiView(params);
}

void TestController::updateTable() {
    codeset.setTable(CODE_TEST_BUILDER_LATEST_EXPECTED_ROOM_ID, latestPosition);
    match.builders.access(BUILDER_INDEX, [&](Builder& builder) {
        codeset.setTable(CODE_TEST_BUILDER_LATEST_ACTUAL_ROOM_ID, builder.character.location.roomId);
    });
}

