#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include "src/hider_seeker/api/HiderSeekerService.hpp"
#include "src/hider_seeker/store/FileStore.hpp"

using namespace hider_seeker;
using json = nlohmann::json;

TEST_CASE("Request validation test cases", "[service_validation]") {
    // Create a FileStore for testing
    FileStore store("var/test_state/hider_seeker_matches");
    
    // Create service
    HiderSeekerService service(store);
    
    // Test unknown intent.type
    json createBody;
    createBody["account"] = "host";
    createBody["hider_count"] = 1;
    createBody["seed"] = 12345;
    
    auto createResponse = service.createMatch(createBody.dump());
    REQUIRE(createResponse.status == 201);
    std::string matchId = createResponse.body["match_id"];
    
    json joinBody;
    joinBody["account"] = "alice";
    joinBody["expected_revision"] = 0;
    joinBody["idempotency_key"] = "join1";
    
    auto joinResponse = service.joinMatch(matchId, joinBody.dump());
    REQUIRE(joinResponse.status == 200);
    
    json startBody;
    startBody["account"] = "host";
    startBody["expected_revision"] = 1;
    startBody["idempotency_key"] = "start1";
    
    auto startResponse = service.startMatch(matchId, startBody.dump());
    REQUIRE(startResponse.status == 200);
    
    // Test unknown intent.type
    json moveBody1;
    moveBody1["account"] = "alice";
    moveBody1["expected_revision"] = 2;
    moveBody1["idempotency_key"] = "move1";
    moveBody1["play_mode"] = "PLAY";
    json intent1;
    intent1["type"] = "UNKNOWN_ACTION"; // Invalid type
    moveBody1["intent"] = intent1;
    
    auto moveResponse1 = service.submitMove(matchId, moveBody1.dump());
    REQUIRE(moveResponse1.status == 400);
    REQUIRE(moveResponse1.body["error"] == 400);
    
    // Test play_mode PASS with non-PASS intent
    json moveBody2;
    moveBody2["account"] = "alice";
    moveBody2["expected_revision"] = 2;
    moveBody2["idempotency_key"] = "move2";
    moveBody2["play_mode"] = "PASS"; // PASS mode
    json intent2;
    intent2["type"] = "CONFIGURE_SERVER"; // Not a PASS intent
    moveBody2["intent"] = intent2;
    
    auto moveResponse2 = service.submitMove(matchId, moveBody2.dump());
    REQUIRE(moveResponse2.status == 400);
    REQUIRE(moveResponse2.body["error"] == 400);
    
    // Test ticks 0 and 17 (MAX_ADVANCE_TICKS is 16)
    json moveBody3;
    moveBody3["account"] = "alice";
    moveBody3["expected_revision"] = 2;
    moveBody3["idempotency_key"] = "move3";
    moveBody3["play_mode"] = "PASS";
    moveBody3["ticks"] = 0; // Invalid - zero ticks
    json intent3;
    intent3["type"] = "PASS";
    moveBody3["intent"] = intent3;
    
    auto moveResponse3 = service.submitMove(matchId, moveBody3.dump());
    REQUIRE(moveResponse3.status == 400);
    REQUIRE(moveResponse3.body["error"] == 400);
    
    json moveBody4;
    moveBody4["account"] = "alice";
    moveBody4["expected_revision"] = 2;
    moveBody4["idempotency_key"] = "move4";
    moveBody4["play_mode"] = "PASS";
    moveBody4["ticks"] = 17; // Invalid - exceeds MAX_ADVANCE_TICKS
    json intent4;
    intent4["type"] = "PASS";
    moveBody4["intent"] = intent4;
    
    auto moveResponse4 = service.submitMove(matchId, moveBody4.dump());
    REQUIRE(moveResponse4.status == 400);
    REQUIRE(moveResponse4.body["error"] == 400);
    
    // Test REMOVE_SERVER without server_id
    json moveBody5;
    moveBody5["account"] = "alice";
    moveBody5["expected_revision"] = 2;
    moveBody5["idempotency_key"] = "move5";
    moveBody5["play_mode"] = "PLAY";
    json intent5;
    intent5["type"] = "REMOVE_SERVER"; // No server_id
    moveBody5["intent"] = intent5;
    
    auto moveResponse5 = service.submitMove(matchId, moveBody5.dump());
    REQUIRE(moveResponse5.status == 400);
    REQUIRE(moveResponse5.body["error"] == 400);
    
    // Test REMOVE_SERVER with invalid server_id (16)
    json moveBody6;
    moveBody6["account"] = "alice";
    moveBody6["expected_revision"] = 2;
    moveBody6["idempotency_key"] = "move6";
    moveBody6["play_mode"] = "PLAY";
    json intent6;
    intent6["type"] = "REMOVE_SERVER";
    intent6["server_id"] = 16; // Invalid server ID
    moveBody6["intent"] = intent6;
    
    auto moveResponse6 = service.submitMove(matchId, moveBody6.dump());
    REQUIRE(moveResponse6.status == 400);
    REQUIRE(moveResponse6.body["error"] == 400);
    
    // Test malformed match ID
    json moveBody7;
    moveBody7["account"] = "alice";
    moveBody7["expected_revision"] = 2;
    moveBody7["idempotency_key"] = "move7";
    moveBody7["play_mode"] = "PLAY";
    json intent7;
    intent7["type"] = "CONFIGURE_SERVER";
    moveBody7["intent"] = intent7;
    
    auto moveResponse7 = service.submitMove("invalid_match_id!", moveBody7.dump());
    REQUIRE(moveResponse7.status == 404);
    REQUIRE(moveResponse7.body["error"] == 404);
    
    // Test non-JSON body
    auto moveResponse8 = service.submitMove(matchId, "not a json body");
    REQUIRE(moveResponse8.status == 400);
    REQUIRE(moveResponse8.body["error"] == 400);
}