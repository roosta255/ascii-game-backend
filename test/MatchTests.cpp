#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include "Codeset.hpp"
#include "Match.hpp"
#include "GeneratorEnum.hpp"
#include "GeneratorTutorial.hpp"
#include "Cardinal.hpp"
#include "Dungeon.hpp"
#include "Room.hpp"
#include "Character.hpp"
#include "CodeEnum.hpp"
#include "Match.hpp"
#include "RoleEnum.hpp"

TEST_CASE("Match instantiation", "[match]") {
    Match match;

    REQUIRE(true);
}

TEST_CASE("Match generation", "[match]") {
    CodeEnum error = CODE_UNKNOWN_ERROR;

    Match match;
    match.host = "host_001";
    match.username = "match_001";
    match.generator = GENERATOR_TUTORIAL;

    Codeset codeset;
    bool isSuccess = match.generate(0, codeset);
    REQUIRE(codeset.findErrorInTable() == CODE_UNSET);
    REQUIRE(isSuccess);

    REQUIRE(match.titan.player.account == "");
    REQUIRE(match.builders.access(0, [&](Builder& builder) {
        REQUIRE(builder.player.account == match.host);
    }));
}

CodeEnum getTogglerOffset(Match& match, int roomId, int& offset) {
    CodeEnum error = CODE_SUCCESS;
    match.dungeon.getRoom(roomId, error).access([&](Room& room){
        room.forEachCharacter(match, [&](Character& character){
            if (character.role == ROLE_TOGGLER) {
                match.containsCharacter(character, offset);
            }
        });
    });
    return error;
}


TEST_CASE("Test Character Animation", "[walking]") {
    CodeEnum error = CODE_UNKNOWN_ERROR;

    // Setup
    Match match;
    CodeEnum result = CODE_SUCCESS;
    const std::string builderId = "builder_1";
    const std::string hostId = builderId;

    match.host = hostId;
    match.filename = "match_002";
    match.generator = GENERATOR_TUTORIAL;

    // Generate tutorial layout
    Codeset codeset;
    bool isSuccess = match.generate(0, codeset);
    REQUIRE(codeset.findErrorInTable() == CODE_UNSET);
    REQUIRE(isSuccess);

    // Start the match
    REQUIRE(match.start());

    // Get the actual character offset of the first builder
    int builderOffset;
    REQUIRE(match.builders.access(0, [&](Builder& builder) {
        REQUIRE(builder.character.keyframes.access(0, [&](const Keyframe& keyframe){
            REQUIRE(keyframe.animation == ANIMATION_NIL);
            REQUIRE(keyframe.t0 == Timestamp::nil());
            REQUIRE(keyframe.t1 == Timestamp::nil());
            REQUIRE(keyframe.isAvailable());
        }));

        for (const auto& keyframe: builder.character.keyframes) {
            REQUIRE(keyframe.animation == ANIMATION_NIL);
            REQUIRE(keyframe.t0 == Timestamp::nil());
            REQUIRE(keyframe.t1 == Timestamp::nil());
            REQUIRE(keyframe.isAvailable() == true);
        }

        REQUIRE(match.containsCharacter(builder.character, builderOffset));
        const auto t0 = Timestamp::buildTimestamp(2200);
        const auto t1 = t0 + Match::MOVE_ANIMATION_DURATION;

        // walk the character to wall
        result = match.moveCharacterToWall(0, builderOffset, Cardinal::east(), t0);
        REQUIRE(result == CODE_SUCCESS);

        const auto it = std::find_if(builder.character.keyframes.begin(), builder.character.keyframes.end(),
            [](const Keyframe& keyframe){return keyframe.animation == ANIMATION_WALKING_FROM_FLOOR_TO_WALL;});

        REQUIRE(it != builder.character.keyframes.end());
        REQUIRE(it->t0 == t0);
        REQUIRE(it->t1 == t1);
    }));
}

TEST_CASE("Tutorial sequence completion", "[match][tutorial]") {
    CodeEnum error = CODE_UNKNOWN_ERROR;

    // Setup
    Match match;
    CodeEnum result = CODE_SUCCESS;
    bool isSuccess = false;
    const std::string builderId = "builder_1";
    const std::string hostId = builderId;

    match.host = hostId;
    match.filename = "match_001";
    match.generator = GENERATOR_TUTORIAL;

    // Generate tutorial layout
    Codeset codeset;
    isSuccess = match.generate(0, codeset);
    REQUIRE(codeset.findErrorInTable() == CODE_UNSET);
    REQUIRE(isSuccess);

    // Start the match
    REQUIRE(match.start());
    Timestamp now;

    // Get the actual character offset of the first builder
    int builderOffset;
    Inventory* inventory;
    REQUIRE(match.builders.access(0, [&](Builder& builder) {
        REQUIRE(match.containsCharacter(builder.character, builderOffset));
        inventory = &builder.player.inventory;
    }));

    REQUIRE(inventory->makeDigest().isEmpty == true);
    REQUIRE(inventory->makeDigest().keys == 0);

    // Get the actual character offset of the togglers
    int toggler1Offset;
    REQUIRE(getTogglerOffset(match, 1, toggler1Offset) == CODE_SUCCESS);

    int toggler3Offset;
    REQUIRE(getTogglerOffset(match, 3, toggler3Offset) == CODE_SUCCESS);

    int toggler9Offset;
    REQUIRE(getTogglerOffset(match, 9, toggler9Offset) == CODE_SUCCESS);

    // Execute the tutorial sequence

    result = match.moveCharacterToWall(0, builderOffset, Cardinal::east(), Timestamp::nil());
    REQUIRE(result == CODE_SUCCESS);

    isSuccess = match.activateCharacter(builderId, toggler1Offset, 1, builderOffset, result);
    REQUIRE(result == CODE_SUCCESS);
    REQUIRE(isSuccess == true);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    result = match.moveCharacterToWall(1, builderOffset, Cardinal::north(), Timestamp::nil());
    REQUIRE(result == CODE_SUCCESS);

    isSuccess = match.activateCharacter(builderId, toggler9Offset, 9, builderOffset, result);
    REQUIRE(result == CODE_SUCCESS);
    REQUIRE(isSuccess == true);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    result = match.moveCharacterToWall(9, builderOffset, Cardinal::east(), Timestamp::nil());
    REQUIRE(result == CODE_SUCCESS);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    result = match.moveCharacterToWall(10, builderOffset, Cardinal::south(), Timestamp::nil());
    REQUIRE(result == CODE_SUCCESS);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    result = match.moveCharacterToWall(2, builderOffset, Cardinal::east(), Timestamp::nil());
    REQUIRE(result == CODE_SUCCESS);

    isSuccess = match.activateCharacter(builderId, toggler3Offset, 3, builderOffset, result);
    REQUIRE(result == CODE_SUCCESS);
    REQUIRE(isSuccess == true);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    result = match.moveCharacterToWall(2, builderOffset, Cardinal::north(), Timestamp::nil());
    REQUIRE(result == CODE_SUCCESS);

    // Take keeper's key between rooms 2 and 3
    isSuccess = match.activateLock(builderId, builderOffset, 2, Cardinal::east().getIndex(), result);
    REQUIRE(result == CODE_SUCCESS);
    REQUIRE(isSuccess == true);
    REQUIRE(inventory->makeDigest().isEmpty == false);
    REQUIRE(inventory->makeDigest().keys == 1);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    // Move to room10's east wall (between room10 & room11)
    result = match.moveCharacterToWall(10, builderOffset, Cardinal::east(), Timestamp::nil());
    REQUIRE(result == CODE_SUCCESS);

    // Take key from room10's south keeper
    isSuccess = match.activateLock(builderId, builderOffset, 10, Cardinal::south().getIndex(), result);
    REQUIRE(result == CODE_SUCCESS);
    REQUIRE(isSuccess == true);
    REQUIRE(inventory->makeDigest().isEmpty == false);
    REQUIRE(inventory->makeDigest().keys == 2);

    // Give key to room11's north shifter to open it
    isSuccess = match.activateLock(builderId, builderOffset, 11, Cardinal::north().getIndex(), result);
    REQUIRE(result == CODE_SUCCESS);
    REQUIRE(isSuccess == true);
    REQUIRE(inventory->makeDigest().isEmpty == false);
    REQUIRE(inventory->makeDigest().keys == 1);

    // Move to room11's north wall (between room11 & room19)
    result = match.moveCharacterToWall(11, builderOffset, Cardinal::north(), Timestamp::nil());
    REQUIRE(result == CODE_SUCCESS);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    // Give key to room19's west shifter to open it
    isSuccess = match.activateLock(builderId, builderOffset, 19, Cardinal::west().getIndex(), result);
    REQUIRE(isSuccess == true);
    REQUIRE(result == CODE_SUCCESS);
    REQUIRE(inventory->makeDigest().isEmpty == true);
    REQUIRE(inventory->makeDigest().keys == 0);

    // Move west to room 18
    result = match.moveCharacterToWall(19, builderOffset, Cardinal::west(), Timestamp::nil());
    REQUIRE(result == CODE_SUCCESS);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    // TODO: Need new API to move character to floor position in room 18
    // For now this will fail as character is blocking the shifter door
    // Take key from room18's east shifter (egress side)
    isSuccess = match.activateLock(builderId, builderOffset, 18, Cardinal::east().getIndex(), result);
    REQUIRE(isSuccess == true);
    REQUIRE(result == CODE_SUCCESS);
    REQUIRE(inventory->makeDigest().isEmpty == false);
    REQUIRE(inventory->makeDigest().keys == 1);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    // Give key to room18's south keeper to open it
    isSuccess = match.activateLock(builderId, builderOffset, 18, Cardinal::south().getIndex(), result);
    REQUIRE(isSuccess == true);
    REQUIRE(result == CODE_SUCCESS);
    REQUIRE(true == inventory->makeDigest().isEmpty);
    REQUIRE(0 == inventory->makeDigest().keys);

    // Move south to room 10
    result = match.moveCharacterToWall(18, builderOffset, Cardinal::south(), Timestamp::nil());
    REQUIRE(result == CODE_SUCCESS);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    // Move east to room 11
    result = match.moveCharacterToWall(10, builderOffset, Cardinal::east(), Timestamp::nil());
    REQUIRE(result == CODE_SUCCESS);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    // TODO: Need new API to move character to floor position in room 11
    // For now this will fail as character is blocking the keeper door
    // Take key from room11's north keeper
    isSuccess = match.activateLock(builderId, builderOffset, 11, Cardinal::north().getIndex(), result);
    REQUIRE(isSuccess == true);
    REQUIRE(result == CODE_SUCCESS);
    REQUIRE(inventory->makeDigest().isEmpty == false);
    REQUIRE(inventory->makeDigest().keys == 1);

    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    // Move north to room 18
    result = match.moveCharacterToWall(10, builderOffset, Cardinal::north(), Timestamp::nil());
    REQUIRE(result == CODE_SUCCESS);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    // Move character to room to allow closing of south keeper
    match.moveCharacterToFloor(18, builderOffset, 1, Timestamp::nil(), result);
    REQUIRE(result == CODE_SUCCESS);

    // TODO: Need new API to move character to floor position in room 18
    // Take the second key from room18's south keeper
    isSuccess = match.activateLock(builderId, builderOffset, 18, Cardinal::south().getIndex(), result);
    REQUIRE(isSuccess == true);
    REQUIRE(result == CODE_SUCCESS);
    REQUIRE(inventory->makeDigest().isEmpty == false);
    REQUIRE(inventory->makeDigest().keys == 2);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    // Now with 2 keys, give key to room18's west keyless keeper
    isSuccess = match.activateLock(builderId, builderOffset, 18, Cardinal::west().getIndex(), result);
    REQUIRE(isSuccess == true);
    REQUIRE(result == CODE_SUCCESS);
    REQUIRE(inventory->makeDigest().isEmpty == false);
    REQUIRE(inventory->makeDigest().keys == 1);

    // Move west to room 17
    result = match.moveCharacterToWall(18, builderOffset, Cardinal::west(), Timestamp::nil());
    REQUIRE(result == CODE_SUCCESS);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    // Give key to room17's west keyless keeper
    isSuccess = match.activateLock(builderId, builderOffset, 17, Cardinal::west().getIndex(), result);
    REQUIRE(isSuccess == true);
    REQUIRE(result == CODE_SUCCESS);
    REQUIRE(inventory->makeDigest().isEmpty == true);
    REQUIRE(inventory->makeDigest().keys == 0);

    // Move west to room 16
    result = match.moveCharacterToWall(17, builderOffset, Cardinal::west(), Timestamp::nil());
    REQUIRE(result == CODE_SUCCESS);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    // Move north to room 24
    result = match.moveCharacterToWall(16, builderOffset, Cardinal::north(), Timestamp::nil());
    REQUIRE(result == CODE_SUCCESS);
}

TEST_CASE("Test jailer", "[match][test]") {
    CodeEnum error = CODE_UNKNOWN_ERROR;

    // Setup
    Match match;
    CodeEnum result = CODE_SUCCESS;
    bool isSuccess = false;
    const std::string builderId = "builder_1";
    const std::string hostId = builderId;

    match.host = hostId;
    match.filename = "match_001";
    match.generator = GENERATOR_TEST;

    // Generate test layout
    Codeset codeset;
    isSuccess = match.generate(0, codeset);
    REQUIRE(codeset.findErrorInTable() == CODE_UNSET);
    REQUIRE(isSuccess);

    // Start the match
    REQUIRE(match.start());

    // Get the actual character offset of the first builder
    int builderOffset;
    Inventory* inventory;
    REQUIRE(match.builders.access(0, [&](Builder& builder) {
        REQUIRE(match.containsCharacter(builder.character, builderOffset));
        inventory = &builder.player.inventory;
    }));

    REQUIRE(inventory->makeDigest().isEmpty == true);
    REQUIRE(inventory->makeDigest().keys == 0);

    result = match.moveCharacterToWall(0, builderOffset, Cardinal::east(), Timestamp::nil());
    REQUIRE(result == CODE_SUCCESS);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    match.moveCharacterToFloor(1, builderOffset, 1, Timestamp::nil(), result);
    REQUIRE(result == CODE_SUCCESS);
/*
    result = match.moveCharacterToWall(1, builderOffset, Cardinal::north(), Timestamp::nil());
    REQUIRE(result == CODE_SUCCESS);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    result = match.moveCharacterToWall(9, builderOffset, Cardinal::west(), Timestamp::nil());
    REQUIRE(result == CODE_BLOCKING_DOOR_TYPE);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    // Take keeper's key between rooms 9 and 8
    isSuccess = match.activateLock(builderId, builderOffset, 9, Cardinal::west().getIndex(), result);
    REQUIRE(result == CODE_SUCCESS);
    REQUIRE(isSuccess == true);
    REQUIRE(inventory->makeDigest().isEmpty == false);
    REQUIRE(inventory->makeDigest().keys == 1);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    result = match.moveCharacterToWall(9, builderOffset, Cardinal::north(), Timestamp::nil());
    REQUIRE(result == CODE_SUCCESS);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    result = match.moveCharacterToWall(17, builderOffset, Cardinal::west(), Timestamp::nil());
    REQUIRE(result == CODE_BLOCKING_DOOR_TYPE);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    // Fill jailer between rooms 17 and 16
    isSuccess = match.activateLock(builderId, builderOffset, 17, Cardinal::west().getIndex(), result);
    REQUIRE(result == CODE_SUCCESS);
    REQUIRE(isSuccess == true);
    REQUIRE(inventory->makeDigest().isEmpty == false);
    REQUIRE(inventory->makeDigest().keys == 1);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    result = match.moveCharacterToWall(17, builderOffset, Cardinal::west(), Timestamp::nil());
    REQUIRE(result == CODE_SUCCESS);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);
*/
}

// how does a DOI work?
