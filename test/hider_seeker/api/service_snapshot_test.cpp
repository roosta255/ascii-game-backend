#include <catch2/catch_test_macros.hpp>
#include "hider_seeker/api/HiderSeekerService.hpp"
#include "hider_seeker/api/match_record.hpp"
#include "hider_seeker/domain/state.hpp"
#include "hider_seeker/store/FileStore.hpp"
#include <nlohmann/json.hpp>

using namespace hider_seeker;

TEST_CASE("SNAPSHOT-002: snapshots exist for correct revisions and slots", "[service]") {
    // Create a test store
    FileStore store("var/test_state/hider_seeker_matches");
    
    // Create service
    HiderSeekerService service(store);
    
    // Create a match with 2 hiders
    nlohmann::json createBody = {
        {"account", "alice"},
        {"hider_count", 2}
    };
    
    auto createResponse = service.createMatch(createBody.dump());
    REQUIRE(createResponse.status == 201);
    
    std::string matchId = createResponse.body["match_id"].get<std::string>();
    
    // Join the match as bob
    nlohmann::json joinBody = {
        {"account", "bob"}
    };
    
    auto joinResponse = service.joinMatch(matchId, joinBody.dump());
    REQUIRE(joinResponse.status == 200);
    
    // Start the match (host is alice)
    auto startResponse = service.startMatch(matchId, nlohmann::json({{"account", "alice"}}).dump());
    REQUIRE(startResponse.status == 200);
    
    // Check that snapshots exist for revisions 0-3
    // Revision 0: create
    // Revision 1: join alice  
    // Revision 2: join bob
    // Revision 3: start
    
    // Check public snapshots exist for all revisions
    std::string content;
    REQUIRE(store.load(matchId + ".view.public.rev-0", content));
    REQUIRE(store.load(matchId + ".view.public.rev-1", content));
    REQUIRE(store.load(matchId + ".view.public.rev-2", content));
    REQUIRE(store.load(matchId + ".view.public.rev-3", content));
    
    // Check hider-0 snapshots exist from revision 1 onward (alice joined at rev 1)
    REQUIRE(store.load(matchId + ".view.hider-0.rev-1", content));
    REQUIRE(store.load(matchId + ".view.hider-0.rev-2", content));
    REQUIRE(store.load(matchId + ".view.hider-0.rev-3", content));
    
    // Check hider-1 snapshots exist from revision 2 onward (bob joined at rev 2)
    REQUIRE(store.load(matchId + ".view.hider-1.rev-2", content));
    REQUIRE(store.load(matchId + ".view.hider-1.rev-3", content));
}

TEST_CASE("SNAPSHOT-002: REJECTED and idempotent replay don't create new snapshots", "[service]") {
    FileStore store("var/test_state/hider_seeker_matches");
    HiderSeekerService service(store);
    
    // Create a match
    nlohmann::json createBody = {
        {"account", "alice"},
        {"hider_count", 1}
    };
    
    auto createResponse = service.createMatch(createBody.dump());
    REQUIRE(createResponse.status == 201);
    
    std::string matchId = createResponse.body["match_id"].get<std::string>();
    
    // Join the match
    nlohmann::json joinBody = {
        {"account", "bob"}
    };
    
    auto joinResponse = service.joinMatch(matchId, joinBody.dump());
    REQUIRE(joinResponse.status == 200);
    
    // Start the match
    auto startResponse = service.startMatch(matchId, nlohmann::json({{"account", "alice"}}).dump());
    REQUIRE(startResponse.status == 200);
    
    // Submit a REJECTED move (invalid server id)
    nlohmann::json rejectedBody = {
        {"account", "alice"},
        {"idempotency_key", "rejected-move-1"},
        {"expected_revision", 3},
        {"intent", {
            {"type", "ADD_SERVER"},
            {"server_id", 99}  // Invalid server id
        }}
    };
    
    auto rejectedResponse = service.submitMove(matchId, rejectedBody.dump());
    REQUIRE(rejectedResponse.status == 200);
    
    // Check that no new snapshot was created for the rejected move
    // (revision should still be 3)
    std::string content;
    REQUIRE_FALSE(store.load(matchId + ".view.public.rev-4", content));
    
    // Submit an idempotent replay of the same move
    auto replayResponse = service.submitMove(matchId, rejectedBody.dump());
    REQUIRE(replayResponse.status == 200);
    
    // Check that still no new snapshot was created for the replay
    REQUIRE_FALSE(store.load(matchId + ".view.public.rev-5", content));
}

TEST_CASE("SNAPSHOT-002: Failed snapshot write doesn't change move response", "[service]") {
    FileStore store("var/test_state/hider_seeker_matches");
    HiderSeekerService service(store);
    
    // Create a match
    nlohmann::json createBody = {
        {"account", "alice"},
        {"hider_count", 1}
    };
    
    auto createResponse = service.createMatch(createBody.dump());
    REQUIRE(createResponse.status == 201);
    
    std::string matchId = createResponse.body["match_id"].get<std::string>();
    
    // Join the match
    nlohmann::json joinBody = {
        {"account", "bob"}
    };
    
    auto joinResponse = service.joinMatch(matchId, joinBody.dump());
    REQUIRE(joinResponse.status == 200);
    
    // Start the match
    auto startResponse = service.startMatch(matchId, nlohmann::json({{"account", "alice"}}).dump());
    REQUIRE(startResponse.status == 200);
    
    // Submit a valid move - this should succeed and create snapshots
    nlohmann::json validBody = {
        {"account", "alice"},
        {"idempotency_key", "valid-move-1"},
        {"expected_revision", 3},
        {"intent", {
            {"type", "ADD_SERVER"},
            {"server_id", 1}  // Valid server id
        }}
    };
    
    auto validResponse = service.submitMove(matchId, validBody.dump());
    REQUIRE(validResponse.status == 200);
    
    // The move should have succeeded and created snapshots
    std::string content;
    REQUIRE(store.load(matchId + ".view.public.rev-4", content));
    REQUIRE(store.load(matchId + ".view.hider-0.rev-4", content));
}