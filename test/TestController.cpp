
#include "AnimationConfig.hpp"
#include "ActionFlyweight.hpp"
#include "Array.hpp"
#include "Builder.hpp"
#include "Cardinal.hpp"
#include "Chest.hpp"
#include "Dungeon.hpp"
#include "Inventory.hpp"
#include "InventoryDigest.hpp"
#include "Player.hpp"
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
        match.allocatePlayerInventory(builder.player);
        playerPtr = &builder.player;
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
        .isSkippingAnimations = isSkippingAnimations,
        .isSkippingLogging = isSkippingLogging
    };

    isSuccess = controller.activate(preactivation);
    updateEverything();
}

void TestController::activateChestLock(LockEnum lockType) {
    int containerCharacterId = -1;
    for (const Chest& chest : match.dungeon.chests) {
        if (chest.lock != lockType || chest.containerCharacterId == -1) continue;
        match.findCharacter(containerCharacterId, [&](const Character& c) {
            return c.characterId == chest.containerCharacterId && c.location.roomId == latestPosition;
        });
        if (containerCharacterId != -1) break;
    }

    Preactivation preactivation{
        .action = {
            .type = ACTION_USE_CHEST_LOCK,
            .characterId = builderOffset,
            .roomId = latestPosition,
            .targetCharacterId = containerCharacterId,
        },
        .playerId = BUILDER_ID,
        .isSkippingAnimations = isSkippingAnimations,
        .isSkippingLogging = isSkippingLogging
    };
    isSuccess = controller.activate(preactivation);
    updateEverything();
}

void TestController::activateContainedCharacter(LockEnum lockType, RoleEnum role) {
    int containedCharacterId = -1;
    for (const Chest& chest : match.dungeon.chests) {
        if (chest.lock != lockType || chest.containerCharacterId == -1) continue;
        int containerFound = -1;
        match.findCharacter(containerFound, [&](const Character& c) {
            return c.characterId == chest.containerCharacterId && c.location.roomId == latestPosition;
        });
        if (containerFound == -1) continue;
        CodeEnum chErr = CODE_UNKNOWN_ERROR;
        match.getCharacter(chest.containerCharacterId, chErr).access([&](Character& containerChar) {
            auto inv = containerChar.getInventory(match.dungeon);
            inv.accessItems(ITEM_CONTAINED, [&](const Item& item) {
                if (containedCharacterId != -1) return;
                match.findCharacter(containedCharacterId, [&](const Character& c) {
                    return c.characterId == item.stacks && c.role == role;
                });
            });
        });
        if (containedCharacterId != -1) break;
    }
    activateObjectCharacter(containedCharacterId);
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
        .isSkippingAnimations = isSkippingAnimations,
        .isSkippingLogging = isSkippingLogging
    };

    isSuccess = controller.activate(preactivation);
    if (isSuccess) latestDirection = dir;
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
        .isSkippingAnimations = isSkippingAnimations,
        .isSkippingLogging = isSkippingLogging
    };
    isSuccess = controller.activate(preactivation);
    if (isSuccess) latestDirection = dir;
    updateEverything();
}

void TestController::activateObjectCharacter(RoleEnum role){
    int objectCharacterId = -1;
    match.findCharacter(objectCharacterId, [&](const Character& c){ return c.location.roomId == latestPosition && c.role == role; });
    activateObjectCharacter(objectCharacterId);
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
        .isSkippingAnimations = isSkippingAnimations,
        .isSkippingLogging = isSkippingLogging
    };
    controller.buildActivationContext(preactivation, [&](ActivationContext& ctx){
        testKeyframe = Keyframe::buildTargetKeyframe(AnimationConfig{.animation=ANIMATION_BOUNCE_FROM_FLOOR_TO_LOCK, .act=ANIMATION_ACT_TARGET_TO_SUBJECT}, ctx);
    });
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

Room* TestController::getLatestRoom() {
    Room* output = nullptr;
    match.dungeon.rooms.access(latestPosition, [&](Room& room){
        output = &room;
    });
    return output;
}

void TestController::generate(int seed) {
    isSuccess = controller.generate(seed);
    latestPosition = builderCharacterPtr->location.roomId;
}

void TestController::giveItem(ItemEnum type) {
    auto inv = playerPtr->getInventory(match.dungeon);
    isSuccess = inv.giveItem(type, codeset.error);
    updateInventory();
}

void TestController::lootInventory(RoleEnum role, const ItemEnum& targetItemType) {
    int containerCharacterId = -1;
    match.findCharacter(containerCharacterId, [&](const Character& c){ return c.location.roomId == latestPosition && c.role == role; });
    lootInventory(containerCharacterId, targetItemType);
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
        .isSkippingAnimations = isSkippingAnimations,
        .isSkippingLogging = isSkippingLogging
    };
    CodeEnum chErr = CODE_UNKNOWN_ERROR;
    match.getCharacter(containerCharacterId, chErr).access([&](Character& containerChar) {
        auto inv = containerChar.getInventory(match.dungeon);
        for (int i = 0; i < inv.size; i++) {
            if (inv.items[i].type == targetItemType) {
                preactivation.action.targetItemIndex = containerChar.itemStartIndex + i;
                break;
            }
        }
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
        .isSkippingAnimations = isSkippingAnimations,
        .isSkippingLogging = isSkippingLogging
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
        .isSkippingAnimations = isSkippingAnimations,
        .isSkippingLogging = isSkippingLogging
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
    controller.tickNpcConducts();
    updateInventory();
    updateTable();
}

void TestController::updateInventory(){
    if (playerPtr) {
        auto inv = playerPtr->getInventory(match.dungeon);
        inventory = inv.makeDigest();
    }
}

Match TestController::saveAndLoadMatch() {
    nlohmann::json j = MatchStoreView(match);
    return (Match)j.get<MatchStoreView>();
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

