// SNAPSHOT-002: every revision-advancing service call publishes snapshots
// under the match mutex; rejected moves, idempotent replays, and failed
// snapshot writes never change anything else.

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>
#include "state.hpp"
#include "service_test_support.hpp"

using namespace hs_test;

namespace {

// Names (without ".json") of every snapshot document stored for the match.
std::set<std::string> snapshotNames(ServiceFixture& fx) {
    std::set<std::string> names;
    const std::string prefix = fx.matchId + ".view.";
    for (const auto& entry : std::filesystem::directory_iterator(fx.temp.dir())) {
        const std::string stem = entry.path().stem().string();
        if (stem.compare(0, prefix.size(), prefix) == 0) {
            names.insert(stem);
        }
    }
    return names;
}

std::string snapshotName(ServiceFixture& fx, const std::string& audienceKey, uint64_t revision) {
    return fx.matchId + ".view." + audienceKey + ".rev-" + std::to_string(revision);
}

} // namespace

TEST_CASE("SNAPSHOT-002: create, join, join, start snapshot exactly the bound audiences", "[hider_seeker][api][snapshots]") {
    ServiceFixture fx;
    fx.setupTwoHiders();

    const std::set<std::string> expected = {
        snapshotName(fx, "public", 0),
        snapshotName(fx, "public", 1),
        snapshotName(fx, "public", 2),
        snapshotName(fx, "public", 3),
        snapshotName(fx, "hider-0", 1),
        snapshotName(fx, "hider-0", 2),
        snapshotName(fx, "hider-0", 3),
        snapshotName(fx, "hider-1", 2),
        snapshotName(fx, "hider-1", 3),
    };
    REQUIRE(snapshotNames(fx) == expected);

    // The stored revision-3 views match what getLatestView serves.
    const ApiResponse latest = fx.service.getRevisionView(fx.matchId, "public", "3", "", "");
    REQUIRE(latest.status == 200);
    REQUIRE(latest.body.at("revision") == 3);
    REQUIRE(latest.body.at("phase") == "ACTIVE");
    const ApiResponse lobby = fx.service.getRevisionView(fx.matchId, "public", "0", "", "");
    REQUIRE(lobby.status == 200);
    REQUIRE(lobby.body.at("phase") == "LOBBY");
}

TEST_CASE("SNAPSHOT-002: committed moves and cancels snapshot their new revision", "[hider_seeker][api][snapshots]") {
    ServiceFixture fx;
    fx.setupTwoHiders();

    const ApiResponse queued = fx.queue("alice", intentOf("PAY_GOAL"));
    REQUIRE(queued.status == 200);
    REQUIRE(queued.body.at("status") == "QUEUED");
    const std::set<std::string> afterQueue = snapshotNames(fx);
    REQUIRE(afterQueue.count(snapshotName(fx, "public", 4)) == 1);
    REQUIRE(afterQueue.count(snapshotName(fx, "hider-0", 4)) == 1);
    REQUIRE(afterQueue.count(snapshotName(fx, "hider-1", 4)) == 1);

    const ApiResponse cancelled = fx.cancel("alice", queued.body.at("intent_id").get<std::string>());
    REQUIRE(cancelled.status == 200);
    REQUIRE(cancelled.body.at("revision") == 5);
    const std::set<std::string> afterCancel = snapshotNames(fx);
    REQUIRE(afterCancel.size() == afterQueue.size() + 3);
    REQUIRE(afterCancel.count(snapshotName(fx, "public", 5)) == 1);
    REQUIRE(afterCancel.count(snapshotName(fx, "hider-0", 5)) == 1);
    REQUIRE(afterCancel.count(snapshotName(fx, "hider-1", 5)) == 1);

    REQUIRE(fx.pass("bob", 1).status == 200);
    REQUIRE(snapshotNames(fx).count(snapshotName(fx, "public", 6)) == 1);
}

TEST_CASE("SNAPSHOT-002: REJECTED moves and idempotent replays write no snapshot", "[hider_seeker][api][snapshots]") {
    ServiceFixture fx;
    fx.setupTwoHiders();

    const json body = fx.moveBody("alice", "PLAY", intentOf("ADD_SERVER"));
    const ApiResponse first = fx.submit(body);
    REQUIRE(first.status == 200);
    const std::set<std::string> afterFirst = snapshotNames(fx);

    const ApiResponse replay = fx.submit(body);
    REQUIRE(replay.body == first.body);
    REQUIRE(snapshotNames(fx) == afterFirst);

    const ApiResponse rejected = fx.play("alice", intentOf("ADD_SERVER"));
    REQUIRE(rejected.status == 422);
    REQUIRE(snapshotNames(fx) == afterFirst);
}

TEST_CASE("SNAPSHOT-002: a failed snapshot write never changes the move's response", "[hider_seeker][api][snapshots]") {
    ServiceFixture fx;
    fx.setupTwoHiders();
    ServiceFixture twin;
    twin.setupTwoHiders();

    // An unreadable document already occupies the revision-4 public snapshot path.
    const std::string blocked = snapshotName(fx, "public", 4);
    {
        std::ofstream file(fx.temp.dir() + "/" + blocked + ".json");
        file << "{not json";
    }

    const ApiResponse played = fx.play("alice", intentOf("BOOST_CPU"));
    const ApiResponse expected = twin.play("alice", intentOf("BOOST_CPU"));
    REQUIRE(played.status == 200);
    REQUIRE(played.status == expected.status);
    REQUIRE(played.body == expected.body);
    REQUIRE(played.headers.empty());

    // The move itself is committed, the blocking document untouched, and the
    // remaining snapshot writes still happened.
    REQUIRE(fx.revision() == 4);
    REQUIRE(fx.temp.readRaw(blocked) == "{not json");
    REQUIRE(snapshotNames(fx).count(snapshotName(fx, "hider-0", 4)) == 1);
    REQUIRE(snapshotNames(fx).count(snapshotName(fx, "hider-1", 4)) == 1);

    // No public revision-4 view can be served from the unreadable document.
    requireError(fx.service.getRevisionView(fx.matchId, "public", "4", "", ""), 404, "VIEW_NOT_FOUND");
}
