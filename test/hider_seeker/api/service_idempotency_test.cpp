#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include "src/hider_seeker/api/HiderSeekerService.hpp"
#include "src/hider_seeker/store/FileStore.hpp"

using namespace hider_seeker;
using json = nlohmann::json;

TEST_CASE("Idempotency-key replay, conflict, and per-account isolation", "[service_idempotency]") {
    // Create a FileStore for testing
    FileStore store("var/test_state/hider_seeker_matches");
    
    // Create service
    HiderSeekerService service(store);
    
    // Test createMatch
    json createBody;
    createBody["account"] = "host";
    createBody["hider_count"] = 1;
    createBody["seed"] = 12345;
    
    auto createResponse = service.createMatch(createBody.dump());
    REQUIRE(createResponse.status == 201);
    std::string matchId = createResponse.body["match_id"];
    
    // Test joinMatch
    json joinBody;
    joinBody["account"] = "alice";
    joinBody["expected_revision"] = 0;
    joinBody["idempotency_key"] = "join1";
    
    auto joinResponse = service.joinMatch(matchId, joinBody.dump());
    REQUIRE(joinResponse.status == 200);
    
    // Test startMatch
    json startBody;
    startBody["account"] = "host";
    startBody["expected_revision"] = 1;
    startBody["idempotency_key"] = "start1";
    
    auto startResponse = service.startMatch(matchId, startBody.dump());
    REQUIRE(startResponse.status == 200);
    
    // Test idempotent request
    json moveBody;
    moveBody["account"] = "alice";
    moveBody["expected_revision"] = 2;
    moveBody["idempotency_key"] = "move1";
    moveBody["play_mode"] = "PLAY";
    json intent;
    intent["type"] = "CONFIGURE_SERVER";
    moveBody["intent"] = intent;
    
    auto moveResponse1 = service.submitMove(matchId, moveBody.dump());
    REQUIRE(moveResponse1.status == 200);
    REQUIRE(moveResponse1.body["status"] == "APPLIED");
    
    // Repeat the same request with same key - should return identical result
    auto moveResponse2 = service.submitMove(matchId, moveBody.dump());
    REQUIRE(moveResponse2.status == 200);
    REQUIRE(moveResponse2.body["status"] == "APPLIED");
    REQUIRE(moveResponse2.body == moveResponse1.body);
    
    // Test same key with different body - should return 422 IDEMPOTENCY_KEY_REUSED
    json moveBody2;
    moveBody2["account"] = "alice";
    moveBody2["expected_revision"] = 2;
    moveBody2["idempotency_key"] = "move1"; // Same key
    moveBody2["play_mode"] = "PLAY";
    json intent2;
    intent2["type"] = "BOOST_CPU"; // Different intent
    moveBody2["intent"] = intent2;
    
    auto moveResponse3 = service.submitMove(matchId, moveBody2.dump());
    REQUIRE(moveResponse3.status == 422);
    REQUIRE(moveResponse3.body["error"] == 422);
    
    // Test same key with different account - should be treated as separate request
    json moveBody3;
    moveBody3["account"] = "bob";
    moveBody3["expected_revision"] = 2;
    moveBody3["idempotency_key"] = "move1"; // Same key
    moveBody3["play_mode"] = "PLAY";
    json intent3;
    intent3["type"] = "CONFIGURE_SERVER";
    moveBody3["intent"] = intent3;
    
    auto moveResponse4 = service.submitMove(matchId, moveBody3.dump());
    REQUIRE(moveResponse4.status == 200);
    REQUIRE(moveResponse4.body["status"] == "APPLIED");
}