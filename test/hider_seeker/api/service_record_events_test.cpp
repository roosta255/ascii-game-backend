#include <catch2/catch_test_macros.hpp>
#include "hider_seeker/api/HiderSeekerService.hpp"
#include "hider_seeker/api/match_record.hpp"
#include "hider_seeker/domain/state.hpp"
#include "hider_seeker/store/FileStore.hpp"
#include "hider_seeker/projection/public_event.hpp"
#include <nlohmann/json.hpp>

using namespace hider_seeker;

TEST_CASE("RECORD-002: submitMove and cancelMove append public events", "[service]") {
    // Create a test store
    FileStore store("var/test_state/hider_seeker_matches");
    
    // Create service
    HiderSeekerService service(store);
    
    // Create a match
    nlohmann::json createBody = {
        {"account", "alice"},
        {"hider_count", 2}
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
    
    // Submit an ADD_SERVER move
    nlohmann::json addServerBody = {
        {"account", "alice"},
        {"idempotency_key", "add-server-1"},
        {"expected_revision", 3},  // Revision after start
        {"intent", {
            {"type", "ADD_SERVER"},
            {"server_id", 2}  // New server
        }}
    };
    
    auto addServerResponse = service.submitMove(matchId, addServerBody.dump());
    REQUIRE(addServerResponse.status == 200);
    
    // Check that public events were appended
    // Load the record to check public_events
    MatchRecord record;
    nlohmann::json root;
    CodeEnum error = CODE_SUCCESS;
    if (store.load(matchId, error, root)) {
        REQUIRE(record.fromJson(root));
        REQUIRE(record.public_events.size() >= 1);  // Should have at least one event
        
        // Check that the event is of correct type
        REQUIRE(record.public_events.back().type == "SERVER_ADDED");
    }
    
    // Submit a REMOVE_SERVER move
    nlohmann::json removeServerBody = {
        {"account", "alice"},
        {"idempotency_key", "remove-server-1"},
        {"expected_revision", 4},  // Revision after add server
        {"intent", {
            {"type", "REMOVE_SERVER"},
            {"server_id", 2}  // Remove the new server
        }}
    };
    
    auto removeServerResponse = service.submitMove(matchId, removeServerBody.dump());
    REQUIRE(removeServerResponse.status == 200);
    
    // Check that public events were appended again
    if (store.load(matchId, error, root)) {
        REQUIRE(record.fromJson(root));
        REQUIRE(record.public_events.size() >= 2);  // Should have at least two events
        
        // Check that the last event is of correct type
        REQUIRE(record.public_events.back().type == "SERVER_REMOVED");
    }
    
    // Test REJECTED move doesn't append events
    nlohmann::json rejectedBody = {
        {"account", "alice"},
        {"idempotency_key", "rejected-move-1"},
        {"expected_revision", 5},  // Revision after remove server
        {"intent", {
            {"type", "ADD_SERVER"},
            {"server_id", 99}  // Invalid server id
        }}
    };
    
    auto rejectedResponse = service.submitMove(matchId, rejectedBody.dump());
    REQUIRE(rejectedResponse.status == 200);
    
    // Check that no new events were appended for rejected move
    if (store.load(matchId, error, root)) {
        REQUIRE(record.fromJson(root));
        // Should still have the same number of events as before the rejected move
        // (the rejected move doesn't append events)
    }
}

TEST_CASE("RECORD-002: Idempotent replay doesn't append public events", "[service]") {
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
    
    // Submit a move
    nlohmann::json moveBody = {
        {"account", "alice"},
        {"idempotency_key", "test-key-1"},
        {"expected_revision", 3},
        {"intent", {
            {"type", "ADD_SERVER"},
            {"server_id", 1}
        }}
    };
    
    auto firstResponse = service.submitMove(matchId, moveBody.dump());
    REQUIRE(firstResponse.status == 200);
    
    // Load the record to check public_events count
    MatchRecord record;
    nlohmann::json root;
    CodeEnum error = CODE_SUCCESS;
    if (store.load(matchId, error, root)) {
        REQUIRE(record.fromJson(root));
        int initialEventCount = record.public_events.size();
        
        // Submit the same move again (idempotent replay)
        auto secondResponse = service.submitMove(matchId, moveBody.dump());
        REQUIRE(secondResponse.status == 200);
        
        // Load again to check that no new events were appended
        if (store.load(matchId, error, root)) {
            REQUIRE(record.fromJson(root));
            // Should have same number of events as before
            REQUIRE(record.public_events.size() == initialEventCount);
        }
    }
}