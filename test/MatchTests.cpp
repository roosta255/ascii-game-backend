#include <catch2/catch_test_macros.hpp>
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

    REQUIRE(match.generate(0, error));

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

TEST_CASE("Tutorial sequence completion", "[match][tutorial]") {
    CodeEnum error = CODE_UNKNOWN_ERROR;

    // Setup
    Match match;
    CodeEnum result = CODE_SUCCESS;
    const std::string builderId = "builder_1";
    const std::string hostId = builderId;

    match.host = hostId;
    match.filename = "match_001";
    match.generator = GENERATOR_TUTORIAL;

    // Generate tutorial layout
    GeneratorTutorial generator;
    REQUIRE(generator.generate(0, match));

    // Start the match
    REQUIRE(match.start());

    // Get the actual character offset of the first builder
    int builderOffset;
    REQUIRE(match.builders.access(0, [&](Builder& builder) {
        REQUIRE(match.containsCharacter(builder.character, builderOffset));
    }));

    // Get the actual character offset of the togglers
    int toggler1Offset;
    REQUIRE(getTogglerOffset(match, 1, toggler1Offset) == CODE_SUCCESS);

    int toggler3Offset;
    REQUIRE(getTogglerOffset(match, 3, toggler3Offset) == CODE_SUCCESS);

    int toggler9Offset;
    REQUIRE(getTogglerOffset(match, 9, toggler9Offset) == CODE_SUCCESS);

    // Execute the tutorial sequence

    result = match.moveCharacterToWall(0, builderOffset, Cardinal::east());
    REQUIRE(result == CODE_SUCCESS);

    result = match.activateCharacter(builderId, toggler1Offset, 1, builderOffset);
    REQUIRE(result == CODE_SUCCESS);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    result = match.moveCharacterToWall(1, builderOffset, Cardinal::north());
    REQUIRE(result == CODE_SUCCESS);

    result = match.activateCharacter(builderId, toggler9Offset, 9, builderOffset);
    REQUIRE(result == CODE_SUCCESS);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    result = match.moveCharacterToWall(9, builderOffset, Cardinal::east());
    REQUIRE(result == CODE_SUCCESS);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    result = match.moveCharacterToWall(10, builderOffset, Cardinal::south());
    REQUIRE(result == CODE_SUCCESS);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    result = match.moveCharacterToWall(2, builderOffset, Cardinal::east());
    REQUIRE(result == CODE_SUCCESS);

    result = match.activateCharacter(builderId, toggler3Offset, 3, builderOffset);
    REQUIRE(result == CODE_SUCCESS);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    result = match.moveCharacterToWall(2, builderOffset, Cardinal::north());
    REQUIRE(result == CODE_SUCCESS);

    // Take keeper's key between rooms 2 and 3
    result = match.activateLock(builderId, builderOffset, 2, Cardinal::east().getIndex());
    REQUIRE(result == CODE_SUCCESS);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    // Move to room10's east wall (between room10 & room11)
    result = match.moveCharacterToWall(10, builderOffset, Cardinal::east());
    REQUIRE(result == CODE_SUCCESS);

    // Take key from room10's south keeper
    result = match.activateLock(builderId, builderOffset, 10, Cardinal::south().getIndex());
    REQUIRE(result == CODE_SUCCESS);

    // Give key to room11's north shifter to open it
    result = match.activateLock(builderId, builderOffset, 11, Cardinal::north().getIndex());
    REQUIRE(result == CODE_SUCCESS);

    // Move to room11's north wall (between room11 & room19)
    result = match.moveCharacterToWall(11, builderOffset, Cardinal::north());
    REQUIRE(result == CODE_SUCCESS);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    // Give key to room19's west shifter to open it
    result = match.activateLock(builderId, builderOffset, 19, Cardinal::west().getIndex());
    REQUIRE(result == CODE_SUCCESS);

    // Move west to room 18
    result = match.moveCharacterToWall(19, builderOffset, Cardinal::west());
    REQUIRE(result == CODE_SUCCESS);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    // TODO: Need new API to move character to floor position in room 18
    // For now this will fail as character is blocking the shifter door
    // Take key from room18's east shifter (egress side)
    result = match.activateLock(builderId, builderOffset, 18, Cardinal::east().getIndex());
    REQUIRE(result == CODE_SUCCESS);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    // Give key to room18's south keeper to open it
    result = match.activateLock(builderId, builderOffset, 18, Cardinal::south().getIndex());
    REQUIRE(result == CODE_SUCCESS);

    // Move south to room 10
    result = match.moveCharacterToWall(18, builderOffset, Cardinal::south());
    REQUIRE(result == CODE_SUCCESS);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    // Move east to room 11
    result = match.moveCharacterToWall(10, builderOffset, Cardinal::east());
    REQUIRE(result == CODE_SUCCESS);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    // TODO: Need new API to move character to floor position in room 11
    // For now this will fail as character is blocking the keeper door
    // Take key from room11's north keeper
    result = match.activateLock(builderId, builderOffset, 11, Cardinal::north().getIndex());
    REQUIRE(result == CODE_SUCCESS);

    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    // Move north to room 18
    result = match.moveCharacterToWall(10, builderOffset, Cardinal::north());
    REQUIRE(result == CODE_SUCCESS);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    // Move character to room to allow closing of south keeper
    match.moveCharacterToFloor(18, builderOffset, 1, result);
    REQUIRE(result == CODE_SUCCESS);

    // TODO: Need new API to move character to floor position in room 18
    // Take the second key from room18's south keeper
    result = match.activateLock(builderId, builderOffset, 18, Cardinal::south().getIndex());
    REQUIRE(result == CODE_SUCCESS);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    // Now with 2 keys, give key to room18's west keyless keeper
    result = match.activateLock(builderId, builderOffset, 18, Cardinal::west().getIndex());
    REQUIRE(result == CODE_SUCCESS);

    // Move west to room 17
    result = match.moveCharacterToWall(18, builderOffset, Cardinal::west());
    REQUIRE(result == CODE_SUCCESS);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    // Give key to room17's west keyless keeper
    result = match.activateLock(builderId, builderOffset, 17, Cardinal::west().getIndex());
    REQUIRE(result == CODE_SUCCESS);

    // Move west to room 16
    result = match.moveCharacterToWall(17, builderOffset, Cardinal::west());
    REQUIRE(result == CODE_SUCCESS);
    match.endTurn(builderId, result);
    REQUIRE(result == CODE_SUCCESS);

    // Move north to room 24
    result = match.moveCharacterToWall(16, builderOffset, Cardinal::north());
    REQUIRE(result == CODE_SUCCESS);
}

// how does a DOI work?
