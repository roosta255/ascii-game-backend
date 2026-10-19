#include <catch2/catch_test_macros.hpp>
#include "../../../src/hider_seeker/projection/public_event.hpp"
#include "../../../src/hider_seeker/domain/state.hpp"
#include "../../../src/hider_seeker/projection/audience_views.hpp"

using namespace hider_seeker;

TEST_CASE("PublicEventEntry SERVER_PROBED JSON round-trip", "[public_event]") {
    // Test: a SERVER_PROBED PublicEventEntry with server_id 2 and outcome "HIT"
    // round-trips through toJson/json (and its JSON has "server_id": 2 and
    // "outcome": "HIT").
    PublicEventEntry entry;
    entry.type = "SERVER_PROBED";
    entry.tick = 5;
    entry.revision = 100;
    entry.server_id = 2;
    entry.outcome = "HIT";

    const auto json = toJson(entry);
    REQUIRE(json["type"] == "SERVER_PROBED");
    REQUIRE(json["tick"] == 5);
    REQUIRE(json["revision"] == 100);
    REQUIRE(json["server_id"] == 2);
    REQUIRE(json["outcome"] == "HIT");

    // Test round-trip
    PublicEventEntry parsed;
    REQUIRE(fromJson(json, parsed));
    REQUIRE(parsed.type == "SERVER_PROBED");
    REQUIRE(parsed.tick == 5);
    REQUIRE(parsed.revision == 100);
    REQUIRE(parsed.server_id == 2);
    REQUIRE(parsed.outcome == "HIT");
}

TEST_CASE("PublicEventEntry SERVER_ADDED JSON serialization", "[public_event]") {
    // Test: a SERVER_ADDED entry serializes exactly {type, tick, revision}
    // (json.size() == 3, no server_id key).
    PublicEventEntry entry;
    entry.type = "SERVER_ADDED";
    entry.tick = 3;
    entry.revision = 4;

    const auto json = toJson(entry);
    REQUIRE(json.size() == 3);
    REQUIRE(json["type"] == "SERVER_ADDED");
    REQUIRE(json["tick"] == 3);
    REQUIRE(json["revision"] == 4);
    REQUIRE(json.find("server_id") == json.end());
    REQUIRE(json.find("outcome") == json.end());
}

TEST_CASE("PublicEventEntry legacy record loading", "[public_event]") {
    // Test: a stored record without the new keys
    // ({"type":"SERVER_REMOVED","tick":3,"revision":4}) still loads.
    const nlohmann::json legacyJson = {{"type", "SERVER_REMOVED"}, {"tick", 3}, {"revision", 4}};
    
    PublicEventEntry parsed;
    REQUIRE(fromJson(legacyJson, parsed));
    REQUIRE(parsed.type == "SERVER_REMOVED");
    REQUIRE(parsed.tick == 3);
    REQUIRE(parsed.revision == 4);
    REQUIRE(parsed.server_id == 255); // default value
    REQUIRE(parsed.outcome == ""); // default value
}

TEST_CASE("PublicEventEntry SERVER_PROBED invalid outcome rejection", "[public_event]") {
    // Test: a SERVER_PROBED record with outcome "MAYBE" is rejected.
    const nlohmann::json badJson = {{"type", "SERVER_PROBED"}, {"tick", 3}, {"revision", 4},
                                    {"server_id", 2}, {"outcome", "MAYBE"}};
    
    PublicEventEntry parsed;
    REQUIRE_FALSE(fromJson(badJson, parsed));
}

TEST_CASE("VIEW-006: SERVER_PROBED entries in public view JSON", "[public_event][projection]") {
    // Test: a public view whose context holds a SERVER_PROBED entry (server_id 2, HIT)
    // contains {"type": "SERVER_PROBED", "server_id": 2, "outcome": "HIT", ...}
    
    // Create a basic match state and context
    MatchState state{};
    ViewContext ctx;
    ctx.matchId = "test-match";
    ctx.accounts.assignValue(0, "alice");
    ctx.accounts.assignValue(1, "bob");
    
    // Add a SERVER_PROBED event to the context
    PublicEventEntry entry{"SERVER_PROBED", 2, 5, 2, "HIT"};
    ctx.publicEvents.push_back(entry);
    
    const auto pub = buildPublicView(state, ctx);
    
    REQUIRE(pub.at("events").size() == 1);
    REQUIRE(pub.at("events").at(0).at("type") == "SERVER_PROBED");
    REQUIRE(pub.at("events").at(0).at("server_id") == 2);
    REQUIRE(pub.at("events").at(0).at("outcome") == "HIT");
}