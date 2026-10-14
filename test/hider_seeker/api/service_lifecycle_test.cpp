#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include "src/hider_seeker/api/HiderSeekerService.hpp"
#include "src/hider_seeker/store/FileStore.hpp"
#include "src/hider_seeker/domain/serialization.hpp"

using namespace hider_seeker;
using json = nlohmann::json;

TEST_CASE("Full lobby lifecycle test", "[service_lifecycle]") {
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
    REQUIRE(createResponse.body.contains("match_id"));
    REQUIRE(createResponse.body["revision"] == 0);
    REQUIRE(createResponse.body["phase"] == "LOBBY");
    
    std::string matchId = createResponse.body["match_id"];
    
    // Test joinMatch for first hider
    json joinBody1;
    joinBody1["account"] = "alice";
    joinBody1["expected_revision"] = 0;
    joinBody1["idempotency_key"] = "join1";
    
    auto joinResponse1 = service.joinMatch(matchId, joinBody1.dump());
    REQUIRE(joinResponse1.status == 200);
    REQUIRE(joinResponse1.body["hider_id"] == 0);
    REQUIRE(joinResponse1.body["revision"] == 1);
    
    // Test joinMatch for second hider
    json joinBody2;
    joinBody2["account"] = "bob";
    joinBody2["expected_revision"] = 1;
    joinBody2["idempotency_key"] = "join2";
    
    auto joinResponse2 = service.joinMatch(matchId, joinBody2.dump());
    REQUIRE(joinResponse2.status == 200);
    REQUIRE(joinResponse2.body["hider_id"] == 1);
    REQUIRE(joinResponse2.body["revision"] == 2);
    
    // Test joinMatch for third hider (should fail - match full)
    json joinBody3;
    joinBody3["account"] = "charlie";
    joinBody3["expected_revision"] = 2;
    joinBody3["idempotency_key"] = "join3";
    
    auto joinResponse3 = service.joinMatch(matchId, joinBody3.dump());
    REQUIRE(joinResponse3.status == 409);
    REQUIRE(joinResponse3.body["error"] == 409);
    
    // Test startMatch
    json startBody;
    startBody["account"] = "host";
    startBody["expected_revision"] = 2;
    startBody["idempotency_key"] = "start1";
    
    auto startResponse = service.startMatch(matchId, startBody.dump());
    REQUIRE(startResponse.status == 200);
    REQUIRE(startResponse.body["revision"] == 3);
    REQUIRE(startResponse.body["phase"] == "ACTIVE");
    
    // Test getMatch
    auto getResponse = service.getMatch(matchId);
    REQUIRE(getResponse.status == 200);
    REQUIRE(getResponse.body["match_id"] == matchId);
    REQUIRE(getResponse.body["host"] == "host");
    REQUIRE(getResponse.body["phase"] == "ACTIVE");
    REQUIRE(getResponse.body["revision"] == 3);
    REQUIRE(getResponse.body.contains("hiders"));
    
    // Test that hiders array has correct structure
    auto hiders = getResponse.body["hiders"];
    REQUIRE(hiders.size() == 2);
    REQUIRE(hiders[0]["hider_id"] == 0);
    REQUIRE(hiders[0]["account"] == "alice");
    REQUIRE(hiders[1]["hider_id"] == 1);
    REQUIRE(hiders[1]["account"] == "bob");
    
    // Test that repeated join by same account fails
    json joinBody4;
    joinBody4["account"] = "alice";
    joinBody4["expected_revision"] = 3;
    joinBody4["idempotency_key"] = "join4";
    
    auto joinResponse4 = service.joinMatch(matchId, joinBody4.dump());
    REQUIRE(joinResponse4.status == 409);
    REQUIRE(joinResponse4.body["error"] == 409);
    
    // Test that start by non-host fails
    json startBody2;
    startBody2["account"] = "alice";
    startBody2["expected_revision"] = 3;
    startBody2["idempotency_key"] = "start2";
    
    auto startResponse2 = service.startMatch(matchId, startBody2.dump());
    REQUIRE(startResponse2.status == 403);
    REQUIRE(startResponse2.body["error"] == 403);
    
    // Test that start with no hiders fails
    json createBody2;
    createBody2["account"] = "host2";
    createBody2["hider_count"] = 1;
    createBody2["seed"] = 54321;
    
    auto createResponse2 = service.createMatch(createBody2.dump());
    REQUIRE(createResponse2.status == 201);
    std::string matchId2 = createResponse2.body["match_id"];
    
    json startBody3;
    startBody3["account"] = "host2";
    startBody3["expected_revision"] = 0;
    startBody3["idempotency_key"] = "start3";
    
    auto startResponse3 = service.startMatch(matchId2, startBody3.dump());
    REQUIRE(startResponse3.status == 409);
    REQUIRE(startResponse3.body["error"] == 409);
    
    // Test that join after start fails
    json joinBody5;
    joinBody5["account"] = "charlie";
    joinBody5["expected_revision"] = 3;
    joinBody5["idempotency_key"] = "join5";
    
    auto joinResponse5 = service.joinMatch(matchId, joinBody5.dump());
    REQUIRE(joinResponse5.status == 409);
    REQUIRE(joinResponse5.body["error"] == 409);
}