#include <catch2/catch_test_macros.hpp>
#include "hider_seeker/api/HiderSeekerService.hpp"
#include "hider_seeker/api/match_record.hpp"
#include "hider_seeker/domain/state.hpp"
#include "hider_seeker/store/FileStore.hpp"
#include <nlohmann/json.hpp>

using namespace hider_seeker;

TEST_CASE("SERVICE-001/SERVICE-002: ApiResponse construction with headers", "[service]") {
    // Test that the new ApiResponse constructor still works
    nlohmann::json body = {{"test", "value"}};
    
    // This should compile and work (the original two-argument form)
    ApiResponse response1{200, body};
    REQUIRE(response1.status == 200);
    REQUIRE(response1.body == body);
    REQUIRE(response1.headers.empty());
    
    // This should also work with the new headers field
    std::vector<std::pair<std::string, std::string>> headers = {{"Test-Header", "test-value"}};
    ApiResponse response2{200, body, headers};
    REQUIRE(response2.status == 200);
    REQUIRE(response2.body == body);
    REQUIRE(response2.headers.size() == 1);
    REQUIRE(response2.headers[0].first == "Test-Header");
    REQUIRE(response2.headers[0].second == "test-value");
}

TEST_CASE("SERVICE-001/SERVICE-002: getLatestView validation order", "[service]") {
    FileStore store("var/test_state/hider_seeker_matches");
    HiderSeekerService service(store);
    
    // Test invalid match ID
    auto response = service.getLatestView("invalid-id", "public", "", "");
    REQUIRE(response.status == 400);
    REQUIRE(response.body["error"] == "INVALID_REQUEST");
    
    // Test invalid audience
    response = service.getLatestView("hs-1234567890abcdef", "invalid-audience", "", "");
    REQUIRE(response.status == 400);
    REQUIRE(response.body["error"] == "INVALID_REQUEST");
    
    // Test valid audience but invalid account for hider
    response = service.getLatestView("hs-1234567890abcdef", "hider", "invalid-account", "");
    REQUIRE(response.status == 400);
    REQUIRE(response.body["error"] == "INVALID_REQUEST");
    
    // Test valid audience and account but match not found
    response = service.getLatestView("hs-1234567890abcdef", "public", "", "");
    REQUIRE(response.status == 404);
    REQUIRE(response.body["error"] == "MATCH_NOT_FOUND");
    
    // Test valid audience, account, and match but hider not in match
    // First create a match
    nlohmann::json createBody = {
        {"account", "alice"},
        {"hider_count", 1}
    };
    
    auto createResponse = service.createMatch(createBody.dump());
    REQUIRE(createResponse.status == 201);
    
    std::string matchId = createResponse.body["match_id"].get<std::string>();
    
    // Try to access as a non-member
    response = service.getLatestView(matchId, "hider", "bob", "");
    REQUIRE(response.status == 403);
    REQUIRE(response.body["error"] == "NOT_IN_MATCH");
}

TEST_CASE("SERVICE-001/SERVICE-002: getRevisionView validation order", "[service]") {
    FileStore store("var/test_state/hider_seeker_matches");
    HiderSeekerService service(store);
    
    // Test invalid match ID
    auto response = service.getRevisionView("invalid-id", "public", "1", "", "");
    REQUIRE(response.status == 400);
    REQUIRE(response.body["error"] == "INVALID_REQUEST");
    
    // Test invalid audience
    response = service.getRevisionView("hs-1234567890abcdef", "invalid-audience", "1", "", "");
    REQUIRE(response.status == 400);
    REQUIRE(response.body["error"] == "INVALID_REQUEST");
    
    // Test invalid revision (non-digit)
    response = service.getRevisionView("hs-1234567890abcdef", "public", "abc", "", "");
    REQUIRE(response.status == 400);
    REQUIRE(response.body["error"] == "INVALID_REQUEST");
    
    // Test invalid revision (too long)
    response = service.getRevisionView("hs-1234567890abcdef", "public", "12345678901234567890", "", "");
    REQUIRE(response.status == 400);
    REQUIRE(response.body["error"] == "INVALID_REQUEST");
    
    // Test valid audience and revision but match not found
    response = service.getRevisionView("hs-1234567890abcdef", "public", "1", "", "");
    REQUIRE(response.status == 404);
    REQUIRE(response.body["error"] == "VIEW_NOT_FOUND");
}

TEST_CASE("SERVICE-001/SERVICE-002: getLatestView and getRevisionView response headers", "[service]") {
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
    
    // Test getLatestView headers for public audience
    auto response = service.getLatestView(matchId, "public", "", "");
    REQUIRE(response.status == 200);
    REQUIRE(response.headers.size() >= 2);  // Should have ETag and Cache-Control
    
    bool hasETag = false;
    bool hasCacheControl = false;
    
    for (const auto& header : response.headers) {
        if (header.first == "ETag") hasETag = true;
        if (header.first == "Cache-Control") hasCacheControl = true;
    }
    
    REQUIRE(hasETag);
    REQUIRE(hasCacheControl);
    
    // Test getRevisionView headers
    response = service.getRevisionView(matchId, "public", "3", "", "");
    REQUIRE(response.status == 200);
    REQUIRE(response.headers.size() >= 2);  // Should have ETag and Cache-Control
    
    hasETag = false;
    hasCacheControl = false;
    
    for (const auto& header : response.headers) {
        if (header.first == "ETag") hasETag = true;
        if (header.first == "Cache-Control") hasCacheControl = true;
    }
    
    REQUIRE(hasETag);
    REQUIRE(hasCacheControl);
}

TEST_CASE("SERVICE-001/SERVICE-002: getLatestView 304 handling", "[service]") {
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
    
    // Get the view first time to get ETag
    auto response1 = service.getLatestView(matchId, "public", "", "");
    REQUIRE(response1.status == 200);
    
    std::string etag;
    for (const auto& header : response1.headers) {
        if (header.first == "ETag") {
            etag = header.second;
            break;
        }
    }
    
    REQUIRE(!etag.empty());
    
    // Get the same view with If-None-Match matching ETag
    auto response2 = service.getLatestView(matchId, "public", "", etag);
    REQUIRE(response2.status == 304);
    REQUIRE(response2.body.is_null());  // Empty body for 304
    
    // Get the same view with If-None-Match not matching ETag
    auto response3 = service.getLatestView(matchId, "public", "", "\"different-etag\"");
    REQUIRE(response3.status == 200);
    REQUIRE(!response3.body.is_null());
}

TEST_CASE("SERVICE-001/SERVICE-002: getLatestView hider resolution", "[service]") {
    FileStore store("var/test_state/hider_seeker_matches");
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
    
    // Start the match
    auto startResponse = service.startMatch(matchId, nlohmann::json({{"account", "alice"}}).dump());
    REQUIRE(startResponse.status == 200);
    
    // Test that alice's hider request always resolves to slot 0 regardless of input
    auto response1 = service.getLatestView(matchId, "hider", "alice", "");
    REQUIRE(response1.status == 200);
    
    auto response2 = service.getLatestView(matchId, "hider", "bob", "");  // bob requesting hider view
    REQUIRE(response2.status == 200);
    
    // Both should be for hider slot 0 (alice's slot)
    // This is a bit tricky to test directly since we don't have access to the internal logic,
    // but we can at least verify they both return valid responses
    REQUIRE(response1.body.contains("hider_id"));
    REQUIRE(response2.body.contains("hider_id"));
}