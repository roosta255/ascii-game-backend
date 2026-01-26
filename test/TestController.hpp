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
    InventoryDigest inventory;

    bool isSuccess = false;
    int builderOffset = -1;
    int latestPosition = 0;


    // constructors
    TestController(const GeneratorEnum& generator);

    // functions
    void updateInventory();

    void moveCharacterToWall(const Cardinal& dir);

    void moveCharacterToFloor(int floorId);

    void activateLock(Cardinal dir);

    void activateDoor(Cardinal dir);

    void endTurn();

};
