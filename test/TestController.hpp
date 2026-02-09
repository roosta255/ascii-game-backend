#pragma once

#include "Cardinal.hpp"
#include "Codeset.hpp"
#include "GeneratorEnum.hpp"
#include "InventoryDigest.hpp"
#include "Match.hpp"

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

    bool isSuccess = false;
    int builderOffset = -1;
    int latestPosition = 0;
    bool isSkippingAnimations = false;


    // constructors
    TestController(const GeneratorEnum& generator);

    // functions
    void activateCharacter(int subjectCharacterId, int objectCharacterId);
    void activateLock(Cardinal dir);
    void activateDoor(Cardinal dir);
    void activateObjectCharacter(int objectCharacterId);

    void endTurn();

    void moveCharacterToFloor(int roomId, int floorId);
    void moveCharacterToFloor(int floorId);
    void moveCharacterToWall(int roomId, const Cardinal& dir);
    void moveCharacterToWall(const Cardinal& dir);

    void updateEverything();
    void updateInventory();
    void updateTable();

};
