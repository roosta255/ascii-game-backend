#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include "src/hider_seeker/api/HiderSeekerService.hpp"
#include "src/hider_seeker/store/FileStore.hpp"

using namespace hider_seeker;
using json = nlohmann::json;

TEST_CASE("QUEUE/PASS play modes and cancelMove test", "[service_queue_pass_cancel]") {
    // Create a FileStore for testing
    FileStore store("var/test_state/hider_seeker_matches");
    
    // Create service
    HiderSeekerService service(store);
    
    // Test createMatch
    json createBody;
    createBody["account"] = "host";
    createBody["hider_count"] = 2;
    createBody["seed"] = 12345;
    
    auto createResponse = service.createMatch(createBody.dump());
    REQUIRE(createResponse.status == 201);
    std::string matchId = createResponse.body["match_id"];
    
    // Test joinMatch for alice
    json joinBody1;
    joinBody1["account"] = "alice";
    joinBody1["expected_revision"] = 0;
    joinBody1["idempotency_key"] = "join1";
    
    auto joinResponse1 = service.joinMatch(matchId, joinBody1.dump());
    REQUIRE(joinResponse1.status == 200);
    
    // Test joinMatch for bob
    json joinBody2;
    joinBody2["account"] = "bob";
    joinBody2["expected_revision"] = 1;
    joinBody2["idempotency_key"] = "join2";
    
    auto joinResponse2 = service.joinMatch(matchId, joinBody2.dump());
    REQUIRE(joinResponse2.status == 200);
    
    // Test startMatch
    json startBody;
    startBody["account"] = "host";
    startBody["expected_revision"] = 2;
    startBody["idempotency_key"] = "start1";
    
    auto startResponse = service.startMatch(matchId, startBody.dump());
    REQUIRE(startResponse.status == 200);
    
    // Test alice queues CONFIGURE_SERVER (cost 5) - should be queued
    json moveBody1;
    moveBody1["account"] = "alice";
    moveBody1["expected_revision"] = 3;
    moveBody1["idempotency_key"] = "move1";
    moveBody1["play_mode"] = "QUEUE";
    json intent1;
    intent1["type"] = "CONFIGURE_SERVER";
    moveBody1["intent"] = intent1;
    
    auto moveResponse1 = service.submitMove(matchId, moveBody1.dump());
    REQUIRE(moveResponse1.status == 200);
    REQUIRE(moveResponse1.body["status"] == "QUEUED");
    REQUIRE(moveResponse1.body["tick"] == 3); // Tick should not advance
    
    // Test bob passes 3 ticks - alice's move should be applied
    json moveBody2;
    moveBody2["account"] = "bob";
    moveBody2["expected_revision"] = 4;
    moveBody2["idempotency_key"] = "move2";
    moveBody2["play_mode"] = "PASS";
    moveBody2["ticks"] = 3;
    
    auto moveResponse2 = service.submitMove(matchId, moveBody2.dump());
    REQUIRE(moveResponse2.status == 200);
    REQUIRE(moveResponse2.body["tick"] == 6); // Tick should advance by 3
    
    // Test alice cancels her queued move
    json cancelBody1;
    cancelBody1["account"] = "alice";
    cancelBody1["expected_revision"] = 5; // This is a placeholder - actual revision would be updated
    cancelBody1["idempotency_key"] = "cancel1";
    
    auto cancelResponse1 = service.cancelMove(matchId, "mv-1", cancelBody1.dump());
    REQUIRE(cancelResponse1.status == 200);
    REQUIRE(cancelResponse1.body["status"] == "CANCELLED");
    
    // Test alice cancels the same move again (should fail)
    auto cancelResponse2 = service.cancelMove(matchId, "mv-1", cancelBody1.dump());
    REQUIRE(cancelResponse2.status == 404);
    REQUIRE(cancelResponse2.body["error"] == 404);
    
    // Test bob cancelling alice's queued move (should fail)
    json cancelBody2;
    cancelBody2["account"] = "bob";
    cancelBody2["expected_revision"] = 5; // This is a placeholder - actual revision would be updated
    cancelBody2["idempotency_key"] = "cancel2";
    
    auto cancelResponse3 = service.cancelMove(matchId, "mv-1", cancelBody2.dump());
    REQUIRE(cancelResponse3.status == 404);
    REQUIRE(cancelResponse3.body["error"] == 404);
    
    // Test alice plays ADD_SERVER (applies now, LOA drops to 0)
    json moveBody3;
    moveBody3["account"] = "alice";
    moveBody3["expected_revision"] = 5;
    moveBody3["idempotency_key"] = "move3";
    moveBody3["play_mode"] = "PLAY";
    json intent3;
    intent3["type"] = "ADD_SERVER";
    moveBody3["intent"] = intent3;
    
    auto moveResponse3 = service.submitMove(matchId, moveBody3.dump());
    REQUIRE(moveResponse3.status == 200);
    REQUIRE(moveResponse3.body["status"] == "APPLIED");
    
    // Test alice queues REMOVE_SERVER of server 1 (QUEUED, 0 LOA)
    json moveBody4;
    moveBody4["account"] = "alice";
    moveBody4["expected_revision"] = 6;
    moveBody4["idempotency_key"] = "move4";
    moveBody4["play_mode"] = "QUEUE";
    json intent4;
    intent4["type"] = "REMOVE_SERVER";
    intent4["server_id"] = 1;
    moveBody4["intent"] = intent4;
    
    auto moveResponse4 = service.submitMove(matchId, moveBody4.dump());
    REQUIRE(moveResponse4.status == 200);
    REQUIRE(moveResponse4.body["status"] == "QUEUED");
    
    // Test bob plays REMOVE_SERVER of server 1 with his own LOA (APPLIED now)
    json moveBody5;
    moveBody5["account"] = "bob";
    moveBody5["expected_revision"] = 7;
    moveBody5["idempotency_key"] = "move5";
    moveBody5["play_mode"] = "PLAY";
    json intent5;
    intent5["type"] = "REMOVE_SERVER";
    intent5["server_id"] = 1;
    moveBody5["intent"] = intent5;
    
    auto moveResponse5 = service.submitMove(matchId, moveBody5.dump());
    REQUIRE(moveResponse5.status == 200);
    REQUIRE(moveResponse5.body["status"] == "APPLIED");
    
    // Test bob passes 1 tick
    json moveBody6;
    moveBody6["account"] = "bob";
    moveBody6["expected_revision"] = 8;
    moveBody6["idempotency_key"] = "move6";
    moveBody6["play_mode"] = "PASS";
    moveBody6["ticks"] = 1;
    
    auto moveResponse6 = service.submitMove(matchId, moveBody6.dump());
    REQUIRE(moveResponse6.status == 200);
}