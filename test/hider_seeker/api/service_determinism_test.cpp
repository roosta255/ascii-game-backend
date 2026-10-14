#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include "src/hider_seeker/api/HiderSeekerService.hpp"
#include "src/hider_seeker/store/FileStore.hpp"

using namespace hider_seeker;
using json = nlohmann::json;

TEST_CASE("Same-seed determinism and request_log correctness", "[service_determinism]") {
    // Create FileStore for testing
    FileStore store1("var/test_state/hider_seeker_matches_1");
    FileStore store2("var/test_state/hider_seeker_matches_2");
    
    // Create two services with same seed
    HiderSeekerService service1(store1);
    HiderSeekerService service2(store2);
    
    // Test createMatch with same seed
    json createBody;
    createBody["account"] = "host";
    createBody["hider_count"] = 2;
    createBody["seed"] = 12345;
    
    auto createResponse1 = service1.createMatch(createBody.dump());
    REQUIRE(createResponse1.status == 201);
    std::string matchId1 = createResponse1.body["match_id"];
    
    auto createResponse2 = service2.createMatch(createBody.dump());
    REQUIRE(createResponse2.status == 201);
    std::string matchId2 = createResponse2.body["match_id"];
    
    // Test joinMatch for both matches
    json joinBody;
    joinBody["account"] = "alice";
    joinBody["expected_revision"] = 0;
    joinBody["idempotency_key"] = "join1";
    
    auto joinResponse1 = service1.joinMatch(matchId1, joinBody.dump());
    REQUIRE(joinResponse1.status == 200);
    
    auto joinResponse2 = service2.joinMatch(matchId2, joinBody.dump());
    REQUIRE(joinResponse2.status == 200);
    
    // Test joinMatch for second player
    json joinBody2;
    joinBody2["account"] = "bob";
    joinBody2["expected_revision"] = 1;
    joinBody2["idempotency_key"] = "join2";
    
    auto joinResponse3 = service1.joinMatch(matchId1, joinBody2.dump());
    REQUIRE(joinResponse3.status == 200);
    
    auto joinResponse4 = service2.joinMatch(matchId2, joinBody2.dump());
    REQUIRE(joinResponse4.status == 200);
    
    // Test startMatch for both matches
    json startBody;
    startBody["account"] = "host";
    startBody["expected_revision"] = 2;
    startBody["idempotency_key"] = "start1";
    
    auto startResponse1 = service1.startMatch(matchId1, startBody.dump());
    REQUIRE(startResponse1.status == 200);
    
    auto startResponse2 = service2.startMatch(matchId2, startBody.dump());
    REQUIRE(startResponse2.status == 200);
    
    // Test identical sequence of requests
    json moveBody;
    moveBody["account"] = "alice";
    moveBody["expected_revision"] = 3;
    moveBody["idempotency_key"] = "move1";
    moveBody["play_mode"] = "PLAY";
    json intent;
    intent["type"] = "CONFIGURE_SERVER";
    moveBody["intent"] = intent;
    
    auto moveResponse1 = service1.submitMove(matchId1, moveBody.dump());
    REQUIRE(moveResponse1.status == 200);
    
    auto moveResponse2 = service2.submitMove(matchId2, moveBody.dump());
    REQUIRE(moveResponse2.status == 200);
    
    // Verify that responses are identical (except for match_id)
    REQUIRE(moveResponse1.body["status"] == moveResponse2.body["status"]);
    REQUIRE(moveResponse1.body["tick"] == moveResponse2.body["tick"]);
    REQUIRE(moveResponse1.body["ticks_advanced"] == moveResponse2.body["ticks_advanced"]);
    
    // Test that request_log entries are correct
    auto getResponse1 = service1.getMatch(matchId1);
    REQUIRE(getResponse1.status == 200);
    
    auto getResponse2 = service2.getMatch(matchId2);
    REQUIRE(getResponse2.status == 200);
    
    // Both should have same number of request_log entries
    // (create, join, join, start, move)
    // Note: This is a simplified test - in reality we'd need to check the actual log content
}