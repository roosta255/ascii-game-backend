#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include "src/hider_seeker/api/HiderSeekerService.hpp"
#include "src/hider_seeker/store/FileStore.hpp"

using namespace hider_seeker;
using json = nlohmann::json;

TEST_CASE("submitMove PLAY-mode immediate application and affordability-driven tick advancement", "[service_submit_move]") {
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
    
    // Test BOOST_CPU (cost 2 action points) - should be applied immediately
    json moveBody1;
    moveBody1["account"] = "alice";
    moveBody1["expected_revision"] = 3;
    moveBody1["idempotency_key"] = "move1";
    moveBody1["play_mode"] = "PLAY";
    json intent1;
    intent1["type"] = "BOOST_CPU";
    moveBody1["intent"] = intent1;
    
    auto moveResponse1 = service.submitMove(matchId, moveBody1.dump());
    REQUIRE(moveResponse1.status == 200);
    REQUIRE(moveResponse1.body["status"] == "APPLIED");
    REQUIRE(moveResponse1.body["ticks_advanced"] == 1); // Should advance 1 tick
    REQUIRE(moveResponse1.body["tick"] == 1);
    
    // Test PAY_GOAL (cost 10 action points) from 0 action points - should advance ticks until affordable
    json moveBody2;
    moveBody2["account"] = "alice";
    moveBody2["expected_revision"] = 4;
    moveBody2["idempotency_key"] = "move2";
    moveBody2["play_mode"] = "PLAY";
    json intent2;
    intent2["type"] = "PAY_GOAL";
    moveBody2["intent"] = intent2;
    
    auto moveResponse2 = service.submitMove(matchId, moveBody2.dump());
    REQUIRE(moveResponse2.status == 200);
    REQUIRE(moveResponse2.body["status"] == "APPLIED");
    REQUIRE(moveResponse2.body["ticks_advanced"] == 5); // Should advance 5 ticks
    REQUIRE(moveResponse2.body["tick"] == 6);
    
    // Test ADD_SERVER twice with 1 LOA - first should be applied immediately, second should be rejected
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
    REQUIRE(moveResponse3.body["tick"] == 7);
    
    // Second ADD_SERVER should be rejected due to lack of LOA
    auto moveResponse4 = service.submitMove(matchId, moveBody3.dump());
    REQUIRE(moveResponse4.status == 422); // REJECTED
    REQUIRE(moveResponse4.body["status"] == "REJECTED");
}