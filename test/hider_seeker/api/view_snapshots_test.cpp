// SNAPSHOT-001: the immutable per-revision view snapshot store.

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "state.hpp"
#include "probe_rules.hpp"
#include "loa_rules.hpp"
#include "audience_views.hpp"
#include "view_snapshots.hpp"
#include "service_test_support.hpp"

using namespace hider_seeker;
using hs_test::TempStore;
using nlohmann::json;

namespace {

const std::string MATCH_ID = "hs-00000000000000aa";

// A 2-slot ACTIVE match at revision 5 with only slot 0 (alice) bound.
MatchRecord sampleRecord() {
    MatchRecord record;
    record.filename = MATCH_ID;
    record.host = "host";
    record.state.schema_version = 2;
    record.state.ruleset_version = 1;
    record.state.revision = 5;
    record.state.tick = 2;
    record.state.phase = PHASE_ACTIVE;
    record.state.hider_count = 2;
    record.state.server_count = 2;
    record.state.hiders[0].active = 1;
    record.state.hiders[0].status = HIDER_STATUS_ACTIVE;
    record.state.hiders[0].server_id = 0;
    record.state.hiders[0].resources[LOA_RESOURCE_INDEX] = 1;
    record.state.hiders[1].server_id = 1;
    record.state.servers[0].active = 1;
    record.state.servers[1].active = 1;
    record.accounts.assignValue(0, "alice");
    record.public_events.push_back(PublicEventEntry{"SERVER_ADDED", 1, 3});
    return record;
}

ViewContext contextOf(const MatchRecord& record) {
    ViewContext ctx;
    ctx.matchId = record.filename;
    ctx.accounts = record.accounts;
    ctx.publicEvents = record.public_events;
    return ctx;
}

std::string pathOf(TempStore& temp, const std::string& name) { return temp.dir() + "/" + name + ".json"; }

bool fileExists(TempStore& temp, const std::string& name) { return std::filesystem::exists(pathOf(temp, name)); }

std::string readFile(TempStore& temp, const std::string& name) {
    std::ifstream file(pathOf(temp, name));
    std::stringstream contents;
    contents << file.rdbuf();
    return contents.str();
}

void writeFile(TempStore& temp, const std::string& name, const std::string& contents) {
    std::filesystem::create_directories(temp.dir());
    std::ofstream file(pathOf(temp, name));
    file << contents;
}

std::set<std::string> keysOf(const json& object) {
    std::set<std::string> keys;
    for (auto it = object.begin(); it != object.end(); ++it) {
        keys.insert(it.key());
    }
    return keys;
}

} // namespace

TEST_CASE("SNAPSHOT-001: publishSnapshots writes the public and every bound hider snapshot", "[hider_seeker][api][snapshots]") {
    TempStore temp;
    const MatchRecord record = sampleRecord();
    const ViewContext ctx = contextOf(record);

    REQUIRE(publishSnapshots(temp.store(), record));

    const std::string publicName = MATCH_ID + ".view.public.rev-5";
    const std::string hider0Name = MATCH_ID + ".view.hider-0.rev-5";
    REQUIRE(fileExists(temp,publicName));
    REQUIRE(fileExists(temp,hider0Name));
    REQUIRE_FALSE(fileExists(temp,MATCH_ID + ".view.hider-1.rev-5")); // slot 1 unbound

    const json publicDoc = json::parse(readFile(temp, publicName));
    REQUIRE(keysOf(publicDoc) == std::set<std::string>{"filename", "version", "view"});
    REQUIRE(publicDoc.at("filename") == publicName);
    REQUIRE(publicDoc.at("version") == 0);
    REQUIRE(publicDoc.at("view") == buildPublicView(record.state, ctx));

    const json hiderDoc = json::parse(readFile(temp, hider0Name));
    REQUIRE(keysOf(hiderDoc) == std::set<std::string>{"filename", "version", "view"});
    REQUIRE(hiderDoc.at("filename") == hider0Name);
    REQUIRE(hiderDoc.at("version") == 0);
    REQUIRE(hiderDoc.at("view") == buildHiderView(record.state, ctx, 0));

    // loadSnapshot serves the "view" value, not the stored wrapper.
    json served;
    REQUIRE(loadSnapshot(temp.store(), MATCH_ID, "public", 5, served));
    REQUIRE(served == publicDoc.at("view"));
    REQUIRE(served.count("filename") == 0);
    json servedHider;
    REQUIRE(loadSnapshot(temp.store(), MATCH_ID, "hider-0", 5, servedHider));
    REQUIRE(servedHider == hiderDoc.at("view"));
}

TEST_CASE("SNAPSHOT-001: republishing a revision never overwrites its snapshots", "[hider_seeker][api][snapshots]") {
    TempStore temp;
    const MatchRecord first = sampleRecord();
    REQUIRE(publishSnapshots(temp.store(), first));

    const std::string publicName = MATCH_ID + ".view.public.rev-5";
    const std::string hider0Name = MATCH_ID + ".view.hider-0.rev-5";
    const std::string publicBytes = readFile(temp, publicName);
    const std::string hiderBytes = readFile(temp, hider0Name);

    MatchRecord second = sampleRecord();
    second.state.tick = 9;
    second.state.servers[0].current.cpu = 200;
    second.state.hiders[0].action_points = 7;
    second.public_events.clear();
    REQUIRE(publishSnapshots(temp.store(), second));

    REQUIRE(readFile(temp, publicName) == publicBytes);
    REQUIRE(readFile(temp, hider0Name) == hiderBytes);

    // writeSnapshotIfAbsent alone leaves an existing document untouched too.
    REQUIRE(writeSnapshotIfAbsent(temp.store(), MATCH_ID, "public", 5, json{{"replaced", true}}));
    REQUIRE(readFile(temp, publicName) == publicBytes);
}

TEST_CASE("SNAPSHOT-001: an unreadable document at a snapshot path is left untouched", "[hider_seeker][api][snapshots]") {
    TempStore temp;
    const std::string publicName = MATCH_ID + ".view.public.rev-5";
    writeFile(temp, publicName, "{not json");

    REQUIRE_FALSE(publishSnapshots(temp.store(), sampleRecord()));
    REQUIRE(readFile(temp, publicName) == "{not json");
    // Every other write was still attempted.
    REQUIRE(fileExists(temp,MATCH_ID + ".view.hider-0.rev-5"));
}

TEST_CASE("SNAPSHOT-001: loadSnapshot is false for a missing or malformed document", "[hider_seeker][api][snapshots]") {
    TempStore temp;
    const json untouched = json{{"untouched", true}};
    json out = untouched;

    SECTION("missing") {
        REQUIRE_FALSE(loadSnapshot(temp.store(), MATCH_ID, "public", 5, out));
        REQUIRE(out == untouched);
    }

    SECTION("malformed") {
        const std::string name = MATCH_ID + ".view.public.rev-5";
        const std::vector<std::string> malformed = {
            json{{"filename", name}, {"version", 1}, {"view", json::object()}}.dump(),
            json{{"filename", name}, {"version", 0}}.dump(),
            json{{"filename", name}, {"version", 0}, {"view", "text"}}.dump(),
            json{{"version", 0}, {"view", json::object()}}.dump(),
            json::array({1, 2}).dump(),
            "{not json"
        };
        for (const std::string& contents : malformed) {
            INFO(contents);
            writeFile(temp, name, contents);
            REQUIRE_FALSE(loadSnapshot(temp.store(), MATCH_ID, "public", 5, out));
            REQUIRE(out == untouched);
        }
    }
}
