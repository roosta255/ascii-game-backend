#include <catch2/catch_test_macros.hpp>
#include "hider_seeker/projection/audience_views.hpp"
#include "hider_seeker/domain/state.hpp"
#include "hider_seeker/api/match_record.hpp"
#include <nlohmann/json.hpp>

using namespace asciigame::hider_seeker;

TEST_CASE("fnv1a64Hex test vectors", "[projection]") {
    REQUIRE(fnv1a64Hex("") == "cbf29ce484222325");
    REQUIRE(fnv1a64Hex("a") == "af63dc4c8601ec8c");
}

TEST_CASE("buildHiderView and buildPublicView exact key sets", "[projection]") {
    // Create a simple match state for testing
    MatchState state;
    state.schema_version = 2;
    state.ruleset_version = 1;
    state.revision = 10;
    state.tick = 5;
    state.phase = PHASE_ACTIVE;
    state.hider_count = 2;
    state.server_count = 2;
    
    // Initialize hiders
    for (int i = 0; i < MAX_HIDERS; ++i) {
        state.hiders[i] = HiderState{};
        state.hiders[i].active = 0;
        state.hiders[i].server_id = NO_SERVER_ID;
    }
    state.hiders[0].active = 1;
    state.hiders[0].status = HIDER_STATUS_ACTIVE;
    state.hiders[0].server_id = 0;
    state.hiders[0].action_points = 5;
    state.hiders[0].resources[LOA_RESOURCE_INDEX] = 100;
    for (int i = 0; i < RESOURCE_TYPE_COUNT; ++i) {
        state.hiders[0].resources[i] = i * 10;
    }
    
    state.hiders[1].active = 1;
    state.hiders[1].status = HIDER_STATUS_CAPTURED;
    state.hiders[1].server_id = 1;
    
    // Initialize servers
    for (int i = 0; i < MAX_SERVERS; ++i) {
        state.servers[i] = ServerState{};
        state.servers[i].active = 0;
    }
    state.servers[0].active = 1;
    state.servers[0].owner_hider_id = 0;
    state.servers[0].current.cpu = 50;
    state.servers[0].current.memory = 25;
    
    state.servers[1].active = 1;
    state.servers[1].owner_hider_id = 1;
    state.servers[1].current.cpu = 75;
    state.servers[1].current.memory = 30;
    
    // Create accounts
    Array<std::string, MAX_HIDERS> accounts;
    accounts[0] = "alice";
    accounts[1] = "bob";
    
    // Create ViewContext
    ViewContext ctx;
    ctx.matchId = "test-match-id";
    ctx.accounts = accounts;
    ctx.publicEvents = {};
    
    // Build hider view for alice (hider 0)
    nlohmann::json hiderView = buildHiderView(state, ctx, 0);
    
    // Check that hider view has exactly the expected keys
    REQUIRE(hiderView.contains("match_id"));
    REQUIRE(hiderView.contains("audience"));
    REQUIRE(hiderView.contains("view_version"));
    REQUIRE(hiderView.contains("schema_version"));
    REQUIRE(hiderView.contains("ruleset_version"));
    REQUIRE(hiderView.contains("revision"));
    REQUIRE(hiderView.contains("tick"));
    REQUIRE(hiderView.contains("phase"));
    REQUIRE(hiderView.contains("hider_count"));
    REQUIRE(hiderView.contains("server_count"));
    REQUIRE(hiderView.contains("servers"));
    REQUIRE(hiderView.contains("events"));
    REQUIRE(hiderView.contains("content_hash"));
    REQUIRE(hiderView.contains("hider_id"));
    REQUIRE(hiderView.contains("self"));
    REQUIRE(hiderView.contains("team"));
    REQUIRE(hiderView.contains("pending"));
    
    // Check that hider view has no extra keys
    std::set<std::string> hiderKeys = {
        "match_id", "audience", "view_version", "schema_version", "ruleset_version",
        "revision", "tick", "phase", "hider_count", "server_count", "servers", 
        "events", "content_hash", "hider_id", "self", "team", "pending"
    };
    
    std::set<std::string> actualHiderKeys;
    for (auto it = hiderView.begin(); it != hiderView.end(); ++it) {
        actualHiderKeys.insert(it.key());
    }
    
    REQUIRE(actualHiderKeys == hiderKeys);
    
    // Build public view
    nlohmann::json publicView = buildPublicView(state, ctx);
    
    // Check that public view has exactly the expected keys (no self, team, pending)
    REQUIRE(publicView.contains("match_id"));
    REQUIRE(publicView.contains("audience"));
    REQUIRE(publicView.contains("view_version"));
    REQUIRE(publicView.contains("schema_version"));
    REQUIRE(publicView.contains("ruleset_version"));
    REQUIRE(publicView.contains("revision"));
    REQUIRE(publicView.contains("tick"));
    REQUIRE(publicView.contains("phase"));
    REQUIRE(publicView.contains("hider_count"));
    REQUIRE(publicView.contains("server_count"));
    REQUIRE(publicView.contains("servers"));
    REQUIRE(publicView.contains("events"));
    REQUIRE(publicView.contains("content_hash"));
    REQUIRE(publicView.contains("hiders"));
    
    // Check that public view has no extra keys
    std::set<std::string> publicKeys = {
        "match_id", "audience", "view_version", "schema_version", "ruleset_version",
        "revision", "tick", "phase", "hider_count", "server_count", "servers", 
        "events", "content_hash", "hiders"
    };
    
    std::set<std::string> actualPublicKeys;
    for (auto it = publicView.begin(); it != publicView.end(); ++it) {
        actualPublicKeys.insert(it.key());
    }
    
    REQUIRE(actualPublicKeys == publicKeys);
    
    // Check that public view has no keys named self, team, pending, or seeker anywhere
    std::string publicJsonStr = publicView.dump();
    REQUIRE(publicJsonStr.find("\"self\"") == std::string::npos);
    REQUIRE(publicJsonStr.find("\"team\"") == std::string::npos);
    REQUIRE(publicJsonStr.find("\"pending\"") == std::string::npos);
    REQUIRE(publicJsonStr.find("\"seeker\"") == std::string::npos);
}

TEST_CASE("buildHiderView phase spellings", "[projection]") {
    MatchState state;
    state.phase = PHASE_LOBBY;
    Array<std::string, MAX_HIDERS> accounts;
    accounts[0] = "alice";
    
    ViewContext ctx;
    ctx.matchId = "test-match-id";
    ctx.accounts = accounts;
    ctx.publicEvents = {};
    
    nlohmann::json view = buildHiderView(state, ctx, 0);
    REQUIRE(view["phase"] == "LOBBY");
    
    state.phase = PHASE_ACTIVE;
    view = buildHiderView(state, ctx, 0);
    REQUIRE(view["phase"] == "ACTIVE");
    
    state.phase = PHASE_COMPLETE;
    view = buildHiderView(state, ctx, 0);
    REQUIRE(view["phase"] == "COMPLETE");
}

TEST_CASE("buildHiderView content_hash determinism", "[projection]") {
    MatchState state;
    state.schema_version = 2;
    state.ruleset_version = 1;
    state.revision = 10;
    state.tick = 5;
    state.phase = PHASE_ACTIVE;
    state.hider_count = 1;
    state.server_count = 1;
    
    // Initialize hiders
    for (int i = 0; i < MAX_HIDERS; ++i) {
        state.hiders[i] = HiderState{};
        state.hiders[i].active = 0;
    }
    state.hiders[0].active = 1;
    state.hiders[0].status = HIDER_STATUS_ACTIVE;
    state.hiders[0].server_id = 0;
    
    // Initialize servers
    for (int i = 0; i < MAX_SERVERS; ++i) {
        state.servers[i] = ServerState{};
        state.servers[i].active = 0;
    }
    state.servers[0].active = 1;
    state.servers[0].owner_hider_id = 0;
    
    Array<std::string, MAX_HIDERS> accounts;
    accounts[0] = "alice";
    
    ViewContext ctx;
    ctx.matchId = "test-match-id";
    ctx.accounts = accounts;
    ctx.publicEvents = {};
    
    nlohmann::json view1 = buildHiderView(state, ctx, 0);
    nlohmann::json view2 = buildHiderView(state, ctx, 0);
    
    REQUIRE(view1["content_hash"] == view2["content_hash"]);
    REQUIRE(view1.dump() == view2.dump());
}