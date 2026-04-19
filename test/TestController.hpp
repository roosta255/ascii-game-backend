#pragma once

#include "Cardinal.hpp"
#include "Codeset.hpp"
#include "GeneratorEnum.hpp"
#include "Keyframe.hpp"
#include "InventoryDigest.hpp"
#include "ItemEnum.hpp"
#include "Match.hpp"
#include "MatchApiParameters.hpp"
#include "MatchApiView.hpp"
#include "Maybe.hpp"
#include "Remodel.hpp"
#include "RoleEnum.hpp"

class Inventory;

class TestController {
public:

    // constants
    static constexpr auto BUILDER_ID = "builder_1";
    static constexpr auto BUILDER_INDEX = 0;
    static constexpr auto MATCH_ID = "builder_1";

    // members
    Match match;
    Codeset codeset;
    MatchController controller;
    Inventory* inventoryPtr = nullptr;
    Character* builderCharacterPtr = nullptr;
    InventoryDigest inventory;
    Maybe<Keyframe> testKeyframe;

    bool isSuccess = false;
    int builderOffset = -1;
    int latestPosition = 0;
    Cardinal latestDirection = Cardinal::north();
    bool isSkippingAnimations = false;
    bool isSkippingLogging = false;

    // constructors
    TestController(const GeneratorEnum& generator);

    // functions
    void activateCharacter(int subjectCharacterId, int objectCharacterId);
    void activateLock(Cardinal dir);
    void activateDoor(Cardinal dir);
    void activateObjectCharacter(int objectCharacterId);
    void activateObjectCharacter(RoleEnum role);

    Remodel buildRemodel(int bossRoomId, std::function<bool(const Match&, const PathfindingCounter&)> acceptance);

    void endTurn();

    void generate(int seed);
    Room* getLatestRoom();
    void giveItem(ItemEnum type);
    void lootInventory(int characterId, const ItemEnum& targetItemType);
    void lootInventory(RoleEnum role, const ItemEnum& targetItemType);

    void moveCharacterToFloor(int roomId, int floorId);
    void moveCharacterToFloor(int floorId);
    void moveCharacterToWall(int roomId, const Cardinal& dir);
    void moveCharacterToWall(const Cardinal& dir);

    void updateEverything();
    void updateInventory();
    void updateTable();

    Match saveAndLoadMatch();
    MatchApiView getMatchApiView();
};
