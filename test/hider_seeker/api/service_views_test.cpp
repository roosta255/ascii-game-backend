// SERVICE-001 / SERVICE-002: ApiResponse headers and the two view endpoints
// (getLatestView / getRevisionView) the controller's view routes expose.

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>
#include "state.hpp"
#include "service_test_support.hpp"

using namespace hs_test;

namespace {

using Headers = std::vector<std::pair<std::string, std::string>>;

const std::string LATEST_CACHE = "no-cache";
const std::string REVISION_CACHE = "private, max-age=31536000, immutable";
const std::string MISSING_MATCH = "hs-ffffffffffffffff";

std::string snapshotPath(ServiceFixture& fx, const std::string& audienceKey, uint64_t revision) {
    return fx.temp.dir() + "/" + fx.matchId + ".view." + audienceKey + ".rev-" + std::to_string(revision) + ".json";
}

std::string etagOf(const ApiResponse& response) {
    return "\"" + response.body.at("content_hash").get<std::string>() + "\"";
}

void requireViewHeaders(const ApiResponse& response, const std::string& etag, const std::string& cacheControl) {
    REQUIRE(response.headers == Headers{{"ETag", etag}, {"Cache-Control", cacheControl}});
}

} // namespace

TEST_CASE("SERVICE-001: two-argument ApiResponse construction defaults headers to empty", "[hider_seeker][api][views]") {
    const ApiResponse response{200, json{{"ok", true}}};
    REQUIRE(response.status == 200);
    REQUIRE(response.body == json{{"ok", true}});
    REQUIRE(response.headers.empty());
}

TEST_CASE("SERVICE-002: view request validation order", "[hider_seeker][api][views]") {
    ServiceFixture fx;
    fx.setupTwoHiders();

    SECTION("malformed match id is checked first") {
        requireError(fx.service.getLatestView("bad id!", "seeker", "", ""), 400, "INVALID_REQUEST");
        requireError(fx.service.getRevisionView("bad id!", "seeker", "abc", "", ""), 400, "INVALID_REQUEST");
        requireError(fx.service.getLatestView("", "public", "", ""), 400, "INVALID_REQUEST");
    }

    SECTION("unknown audience") {
        for (const std::string audience : {"seeker", "hider-1", "HIDER", "", "public "}) {
            INFO(audience);
            requireError(fx.service.getLatestView(fx.matchId, audience, "alice", ""), 400, "INVALID_REQUEST");
            requireError(fx.service.getRevisionView(fx.matchId, audience, "3", "alice", ""), 400, "INVALID_REQUEST");
        }
    }

    SECTION("malformed revision, checked before the account and the match lookup") {
        for (const std::string revision : {"-1", "abc", "", "1.5", " 3", "123456789012345678901"}) {
            INFO(revision);
            requireError(fx.service.getRevisionView(fx.matchId, "public", revision, "", ""), 400, "INVALID_REQUEST");
            requireError(fx.service.getRevisionView(MISSING_MATCH, "hider", revision, "", ""), 400, "INVALID_REQUEST");
        }
    }

    SECTION("hider requires a valid account, checked before the match lookup") {
        for (const std::string account : {"", "bad account!", "alice-1"}) {
            INFO(account);
            requireError(fx.service.getLatestView(fx.matchId, "hider", account, ""), 400, "INVALID_REQUEST");
            requireError(fx.service.getRevisionView(fx.matchId, "hider", "3", account, ""), 400, "INVALID_REQUEST");
            requireError(fx.service.getLatestView(MISSING_MATCH, "hider", account, ""), 400, "INVALID_REQUEST");
        }
    }

    SECTION("public ignores the account entirely") {
        for (const std::string account : {"", "bad account!", "nobody"}) {
            INFO(account);
            REQUIRE(fx.service.getLatestView(fx.matchId, "public", account, "").status == 200);
            REQUIRE(fx.service.getRevisionView(fx.matchId, "public", "3", account, "").status == 200);
        }
    }

    SECTION("unknown match") {
        requireError(fx.service.getLatestView(MISSING_MATCH, "public", "", ""), 404, "MATCH_NOT_FOUND");
        requireError(fx.service.getLatestView(MISSING_MATCH, "hider", "alice", ""), 404, "MATCH_NOT_FOUND");
        requireError(fx.service.getRevisionView(MISSING_MATCH, "public", "0", "", ""), 404, "MATCH_NOT_FOUND");
    }

    SECTION("unreadable match record is a store error, not a missing match") {
        {
            std::ofstream file(fx.temp.dir() + "/" + fx.matchId + ".json", std::ios::trunc);
            file << "{\"filename\": \"" << fx.matchId << "\", \"version\": 3, \"state\": \"zz\"}";
        }
        requireError(fx.service.getLatestView(fx.matchId, "public", "", ""), 500, "STORE_ERROR");
        requireError(fx.service.getLatestView(fx.matchId, "hider", "alice", ""), 500, "STORE_ERROR");
        requireError(fx.service.getRevisionView(fx.matchId, "public", "3", "", ""), 500, "STORE_ERROR");
    }

    SECTION("hider account without a slot") {
        requireError(fx.service.getLatestView(fx.matchId, "hider", "carol", ""), 403, "NOT_IN_MATCH");
        requireError(fx.service.getRevisionView(fx.matchId, "hider", "3", "carol", ""), 403, "NOT_IN_MATCH");
        // The host is not a hider either.
        requireError(fx.service.getLatestView(fx.matchId, "hider", "host", ""), 403, "NOT_IN_MATCH");
    }
}

TEST_CASE("SERVICE-002: getLatestView serves the current revision with no-cache headers", "[hider_seeker][api][views]") {
    ServiceFixture fx;
    fx.setupTwoHiders();

    const ApiResponse pub = fx.service.getLatestView(fx.matchId, "public", "", "");
    REQUIRE(pub.status == 200);
    REQUIRE(pub.body.at("audience") == "public");
    REQUIRE(pub.body.at("revision") == 3);
    requireViewHeaders(pub, etagOf(pub), LATEST_CACHE);

    // alice's "hider" request always resolves to slot 0, bob's to slot 1.
    const ApiResponse alice = fx.service.getLatestView(fx.matchId, "hider", "alice", "");
    REQUIRE(alice.status == 200);
    REQUIRE(alice.body.at("hider_id") == 0);
    REQUIRE(alice.body.at("self").at("account") == "alice");
    requireViewHeaders(alice, etagOf(alice), LATEST_CACHE);
    const ApiResponse bob = fx.service.getLatestView(fx.matchId, "hider", "bob", "");
    REQUIRE(bob.body.at("hider_id") == 1);
    REQUIRE(bob.body.at("self").at("account") == "bob");

    // Served verbatim from the stored snapshot.
    std::ifstream stored(snapshotPath(fx, "hider-0", 3));
    const json document = json::parse(stored);
    REQUIRE(alice.body == document.at("view"));

    SECTION("an existing snapshot is served as stored, not rebuilt") {
        json planted = document;
        planted["view"]["marker"] = "planted";
        planted["view"]["content_hash"] = "0000000000000abc";
        {
            std::ofstream file(snapshotPath(fx, "hider-0", 3));
            file << planted.dump();
        }
        const ApiResponse served = fx.service.getLatestView(fx.matchId, "hider", "alice", "");
        REQUIRE(served.status == 200);
        REQUIRE(served.body == planted.at("view"));
        requireViewHeaders(served, "\"0000000000000abc\"", LATEST_CACHE);
    }

    SECTION("a missing latest snapshot is rebuilt, re-stored, and served identically") {
        REQUIRE(std::filesystem::remove(snapshotPath(fx, "public", 3)));
        const ApiResponse rebuilt = fx.service.getLatestView(fx.matchId, "public", "", "");
        REQUIRE(rebuilt.status == 200);
        REQUIRE(rebuilt.body == pub.body);
        REQUIRE(rebuilt.headers == pub.headers);
        REQUIRE(std::filesystem::exists(snapshotPath(fx, "public", 3)));

        json reloaded;
        REQUIRE(hider_seeker::loadSnapshot(fx.temp.store(), fx.matchId, "public", 3, reloaded));
        REQUIRE(reloaded == pub.body);
    }

    SECTION("the latest view follows new revisions") {
        REQUIRE(fx.play("alice", intentOf("BOOST_CPU")).status == 200);
        const ApiResponse next = fx.service.getLatestView(fx.matchId, "public", "", "");
        REQUIRE(next.body.at("revision") == 4);
        REQUIRE(etagOf(next) != etagOf(pub));
    }
}

TEST_CASE("SERVICE-002: getRevisionView serves stored snapshots only, cached immutably", "[hider_seeker][api][views]") {
    ServiceFixture fx;
    fx.setupTwoHiders();

    const ApiResponse lobby = fx.service.getRevisionView(fx.matchId, "public", "1", "", "");
    REQUIRE(lobby.status == 200);
    REQUIRE(lobby.body.at("revision") == 1);
    REQUIRE(lobby.body.at("phase") == "LOBBY");
    requireViewHeaders(lobby, etagOf(lobby), REVISION_CACHE);

    // Leading zeros name the same revision.
    REQUIRE(fx.service.getRevisionView(fx.matchId, "public", "0001", "", "").body == lobby.body);

    SECTION("future revision") {
        requireError(fx.service.getRevisionView(fx.matchId, "public", "4", "", ""), 404, "VIEW_NOT_FOUND");
        requireError(fx.service.getRevisionView(fx.matchId, "public", "99999999999999999999", "", ""), 404,
                     "VIEW_NOT_FOUND");
    }

    SECTION("revision before the requesting hider joined") {
        requireError(fx.service.getRevisionView(fx.matchId, "hider", "1", "bob", ""), 404, "VIEW_NOT_FOUND");
        requireError(fx.service.getRevisionView(fx.matchId, "hider", "0", "alice", ""), 404, "VIEW_NOT_FOUND");
        REQUIRE(fx.service.getRevisionView(fx.matchId, "hider", "2", "bob", "").status == 200);
        REQUIRE(fx.service.getRevisionView(fx.matchId, "hider", "1", "alice", "").status == 200);
    }

    SECTION("a missing snapshot (pre-milestone-2 data or a failed write) is never rebuilt") {
        REQUIRE(std::filesystem::remove(snapshotPath(fx, "public", 3)));
        requireError(fx.service.getRevisionView(fx.matchId, "public", "3", "", ""), 404, "VIEW_NOT_FOUND");
        REQUIRE_FALSE(std::filesystem::exists(snapshotPath(fx, "public", 3)));
    }

    SECTION("at the current revision it matches getLatestView") {
        const ApiResponse latest = fx.service.getLatestView(fx.matchId, "hider", "alice", "");
        const ApiResponse pinned = fx.service.getRevisionView(fx.matchId, "hider", "3", "alice", "");
        REQUIRE(pinned.status == 200);
        REQUIRE(pinned.body == latest.body);
        REQUIRE(etagOf(pinned) == etagOf(latest));
        requireViewHeaders(pinned, etagOf(latest), REVISION_CACHE);
    }
}

TEST_CASE("SERVICE-002: If-None-Match exact match returns 304", "[hider_seeker][api][views]") {
    ServiceFixture fx;
    fx.setupTwoHiders();

    const ApiResponse latest = fx.service.getLatestView(fx.matchId, "public", "", "");
    const std::string etag = etagOf(latest);

    const ApiResponse notModified = fx.service.getLatestView(fx.matchId, "public", "", etag);
    REQUIRE(notModified.status == 304);
    REQUIRE(notModified.body.is_null());
    REQUIRE(notModified.headers == latest.headers);

    const ApiResponse unquoted = fx.service.getLatestView(fx.matchId, "public", "",
                                                             latest.body.at("content_hash").get<std::string>());
    REQUIRE(unquoted.status == 200);
    REQUIRE(unquoted.body == latest.body);
    REQUIRE(fx.service.getLatestView(fx.matchId, "public", "", "\"other\"").status == 200);

    const ApiResponse pinned = fx.service.getRevisionView(fx.matchId, "public", "3", "", etag);
    REQUIRE(pinned.status == 304);
    REQUIRE(pinned.body.is_null());
    requireViewHeaders(pinned, etag, REVISION_CACHE);
    REQUIRE(fx.service.getRevisionView(fx.matchId, "public", "3", "", "W/" + etag).status == 200);
}
