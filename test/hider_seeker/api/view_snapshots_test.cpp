#include <catch2/catch_test_macros.hpp>
#include "hider_seeker/api/view_snapshots.hpp"
#include "hider_seeker/api/match_record.hpp"
#include "hider_seeker/domain/state.hpp"
#include "hider_seeker/store/FileStore.hpp"
#include "hider_seeker/projection/audience_views.hpp"
#include <nlohmann/json.hpp>

using namespace asciigame::hider_seeker;

TEST_CASE("SNAPSHOT-001: publishSnapshots writes correct files and document structure", "[snapshots]") {
    // Create a test store
    FileStore store("var/test_state/hider_seeker_matches");
    
    // Create a sample match record
    MatchRecord record;
    record.filename = "test-match-id";
    record.version = 1;
    record.host = "test-host";
    record.next_move_id = 1;
    
    // Set up state
    record.state.schema_version = 2;
    record.state.ruleset_version = 1;
    record.state.revision = 5;
    record.state.tick = 10;
    record.state.phase = PHASE_ACTIVE;
    record.state.hider_count = 2;
    record.state.server_count = 2;
    
    // Initialize hiders
    for (int i = 0; i < MAX_HIDERS; ++i) {
        record.state.hiders[i] = HiderState{};
        record.state.hiders[i].active = 0;
    }
    record.state.hiders[0].active = 1;
    record.state.hiders[0].status = HIDER_STATUS_ACTIVE;
    record.state.hiders[0].server_id = 0;
    
    // Initialize servers
    for (int i = 0; i < MAX_SERVERS; ++i) {
        record.state.servers[i] = ServerState{};
        record.state.servers[i].active = 0;
    }
    record.state.servers[0].active = 1;
    record.state.servers[0].owner_hider_id = 0;
    
    // Set up accounts
    for (int i = 0; i < MAX_HIDERS; ++i) {
        record.accounts[i] = "";
    }
    record.accounts[0] = "alice";
    record.accounts[1] = "bob";
    
    // Add some public events
    PublicEventEntry event1;
    event1.type = "SERVER_ADDED";
    event1.tick = 5;
    event1.revision = 5;
    record.appendPublicEvent(event1);
    
    PublicEventEntry event2;
    event2.type = "SERVER_REMOVED";
    event2.tick = 6;
    event2.revision = 5;
    record.appendPublicEvent(event2);
    
    // Publish snapshots
    bool result = publishSnapshots(store, record);
    REQUIRE(result == true);
    
    // Check that the files were created
    std::string publicFilename = "test-match-id.view.public.rev-5";
    std::string hider0Filename = "test-match-id.view.hider-0.rev-5";
    std::string hider1Filename = "test-match-id.view.hider-1.rev-5";
    
    // Check that the public snapshot exists
    std::string content;
    REQUIRE(store.load(publicFilename, content));
    
    // Parse the document
    nlohmann::json doc = nlohmann::json::parse(content);
    REQUIRE(doc.contains("filename"));
    REQUIRE(doc.contains("version"));
    REQUIRE(doc.contains("view"));
    
    REQUIRE(doc["filename"] == publicFilename);
    REQUIRE(doc["version"] == 0);
    
    // Check that the view is a valid JSON object
    REQUIRE(doc["view"].is_object());
    
    // Check that the hider snapshot exists for alice (hider 0)
    REQUIRE(store.load(hider0Filename, content));
    
    doc = nlohmann::json::parse(content);
    REQUIRE(doc.contains("filename"));
    REQUIRE(doc.contains("version"));
    REQUIRE(doc.contains("view"));
    
    REQUIRE(doc["filename"] == hider0Filename);
    REQUIRE(doc["version"] == 0);
    REQUIRE(doc["view"].is_object());
    
    // Check that the hider snapshot for bob (hider 1) exists
    REQUIRE(store.load(hider1Filename, content));
    
    doc = nlohmann::json::parse(content);
    REQUIRE(doc.contains("filename"));
    REQUIRE(doc.contains("version"));
    REQUIRE(doc.contains("view"));
    
    REQUIRE(doc["filename"] == hider1Filename);
    REQUIRE(doc["version"] == 0);
    REQUIRE(doc["view"].is_object());
}

TEST_CASE("SNAPSHOT-001: publishSnapshots never overwrites existing snapshots", "[snapshots]") {
    FileStore store("var/test_state/hider_seeker_matches");
    
    // Create a sample match record
    MatchRecord record;
    record.filename = "test-match-id";
    record.version = 1;
    record.host = "test-host";
    record.next_move_id = 1;
    
    record.state.schema_version = 2;
    record.state.ruleset_version = 1;
    record.state.revision = 5;
    record.state.tick = 10;
    record.state.phase = PHASE_ACTIVE;
    record.state.hider_count = 1;
    record.state.server_count = 1;
    
    // Initialize hiders
    for (int i = 0; i < MAX_HIDERS; ++i) {
        record.state.hiders[i] = HiderState{};
        record.state.hiders[i].active = 0;
    }
    record.state.hiders[0].active = 1;
    
    // Initialize servers
    for (int i = 0; i < MAX_SERVERS; ++i) {
        record.state.servers[i] = ServerState{};
        record.state.servers[i].active = 0;
    }
    record.state.servers[0].active = 1;
    record.state.servers[0].owner_hider_id = 0;
    
    // Set up accounts
    for (int i = 0; i < MAX_HIDERS; ++i) {
        record.accounts[i] = "";
    }
    record.accounts[0] = "alice";
    
    // Publish snapshots first time
    bool result1 = publishSnapshots(store, record);
    REQUIRE(result1 == true);
    
    // Try to publish again with same revision - should not overwrite
    bool result2 = publishSnapshots(store, record);
    REQUIRE(result2 == true);  // Should still return true (no error)
    
    // Check that files exist and are unchanged
    std::string publicFilename = "test-match-id.view.public.rev-5";
    std::string hider0Filename = "test-match-id.view.hider-0.rev-5";
    
    std::string content1, content2;
    REQUIRE(store.load(publicFilename, content1));
    REQUIRE(store.load(hider0Filename, content2));
    
    // Publish again with different record (but same revision) - should not overwrite
    MatchRecord record2 = record;
    // Modify the record to make it different in memory but same revision
    record2.state.tick = 15;  // Different tick
    
    bool result3 = publishSnapshots(store, record2);
    REQUIRE(result3 == true);  // Should still return true (no error)
    
    // Files should still contain original content
    std::string content3, content4;
    REQUIRE(store.load(publicFilename, content3));
    REQUIRE(store.load(hider0Filename, content4));
    
    REQUIRE(content1 == content3);
    REQUIRE(content2 == content4);
}

TEST_CASE("SNAPSHOT-001: loadSnapshot returns correct data", "[snapshots]") {
    FileStore store("var/test_state/hider_seeker_matches");
    
    // Create a sample match record
    MatchRecord record;
    record.filename = "test-match-id";
    record.version = 1;
    record.host = "test-host";
    record.next_move_id = 1;
    
    record.state.schema_version = 2;
    record.state.ruleset_version = 1;
    record.state.revision = 5;
    record.state.tick = 10;
    record.state.phase = PHASE_ACTIVE;
    record.state.hider_count = 1;
    record.state.server_count = 1;
    
    // Initialize hiders
    for (int i = 0; i < MAX_HIDERS; ++i) {
        record.state.hiders[i] = HiderState{};
        record.state.hiders[i].active = 0;
    }
    record.state.hiders[0].active = 1;
    
    // Initialize servers
    for (int i = 0; i < MAX_SERVERS; ++i) {
        record.state.servers[i] = ServerState{};
        record.state.servers[i].active = 0;
    }
    record.state.servers[0].active = 1;
    record.state.servers[0].owner_hider_id = 0;
    
    // Set up accounts
    for (int i = 0; i < MAX_HIDERS; ++i) {
        record.accounts[i] = "";
    }
    record.accounts[0] = "alice";
    
    // Publish snapshots
    bool result = publishSnapshots(store, record);
    REQUIRE(result == true);
    
    // Load the public snapshot
    nlohmann::json view;
    bool loaded = loadSnapshot(store, "test-match-id", "public", 5, view);
    REQUIRE(loaded == true);
    REQUIRE(view.is_object());
    
    // Check that it has the expected keys
    REQUIRE(view.contains("match_id"));
    REQUIRE(view.contains("audience"));
    REQUIRE(view.contains("view_version"));
    REQUIRE(view.contains("schema_version"));
    REQUIRE(view.contains("ruleset_version"));
    REQUIRE(view.contains("revision"));
    REQUIRE(view.contains("tick"));
    REQUIRE(view.contains("phase"));
    REQUIRE(view.contains("hider_count"));
    REQUIRE(view.contains("server_count"));
    REQUIRE(view.contains("servers"));
    REQUIRE(view.contains("events"));
    REQUIRE(view.contains("content_hash"));
    
    // Load the hider snapshot
    nlohmann::json hiderView;
    loaded = loadSnapshot(store, "test-match-id", "hider-0", 5, hiderView);
    REQUIRE(loaded == true);
    REQUIRE(hiderView.is_object());
    
    // Check that it has the expected keys for hider view
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
}

TEST_CASE("SNAPSHOT-001: loadSnapshot returns false for missing or malformed documents", "[snapshots]") {
    FileStore store("var/test_state/hider_seeker_matches");
    
    // Try to load a non-existent snapshot
    nlohmann::json view;
    bool loaded = loadSnapshot(store, "non-existent-match", "public", 1, view);
    REQUIRE(loaded == false);
    
    // Try to load with invalid revision
    loaded = loadSnapshot(store, "test-match-id", "public", 999, view);
    REQUIRE(loaded == false);
}