#include <catch2/catch_test_macros.hpp>
#include <set>
#include <string>
#include "state.hpp"
#include "loa_rules.hpp"
#include "service_test_support.hpp"

using namespace hs_test;
using hider_seeker::LOA_RESOURCE_INDEX;
using hider_seeker::PHASE_ACTIVE;

TEST_CASE("MOVE-002: create, join, and start a match", "[hider_seeker][api][lifecycle]") {
    ServiceFixture fx;

    const ApiResponse created = fx.create("host", 2);
    REQUIRE(created.status == 201);
    REQUIRE(created.body.at("revision") == 0);
    REQUIRE(created.body.at("phase") == "LOBBY");
    const std::string matchId = created.body.at("match_id").get<std::string>();
    REQUIRE(matchId.size() == 19);
    REQUIRE(matchId.compare(0, 3, "hs-") == 0);

    const ApiResponse alice = fx.join("alice");
    REQUIRE(alice.status == 200);
    REQUIRE(alice.body.at("hider_id") == 0);
    REQUIRE(alice.body.at("revision") == 1);

    const ApiResponse bob = fx.join("bob");
    REQUIRE(bob.status == 200);
    REQUIRE(bob.body.at("hider_id") == 1);
    REQUIRE(bob.body.at("revision") == 2);

    SECTION("a third join is MATCH_FULL") {
        requireError(fx.join("carol"), 409, "MATCH_FULL");
    }

    SECTION("a repeated join by a bound account is ALREADY_JOINED") {
        requireError(fx.join("alice"), 409, "ALREADY_JOINED");
    }

    SECTION("start by a non-host account is NOT_HOST") {
        requireError(fx.start("alice"), 403, "NOT_HOST");
    }

    SECTION("start by the host activates the match") {
        const ApiResponse started = fx.start("host");
        REQUIRE(started.status == 200);
        REQUIRE(started.body.at("revision") == 3);
        REQUIRE(started.body.at("phase") == "ACTIVE");

        const auto record = fx.record();
        REQUIRE(record.state.revision == 3);
        REQUIRE(record.state.phase == PHASE_ACTIVE);
        for (int slot = 0; slot < 2; ++slot) {
            REQUIRE(record.state.hiders[slot].active == 1);
            REQUIRE(record.state.hiders[slot].action_points == 0);
            REQUIRE(record.state.hiders[slot].resources[LOA_RESOURCE_INDEX] == 1);
        }

        requireError(fx.join("carol"), 409, "MATCH_NOT_IN_LOBBY");

        const ApiResponse view = fx.getMatch();
        REQUIRE(view.status == 200);
        std::set<std::string> keys;
        for (const auto& item : view.body.items()) {
            keys.insert(item.key());
        }
        REQUIRE(keys == std::set<std::string>{"match_id", "host", "phase", "revision", "tick", "hiders"});
        REQUIRE(view.body.at("match_id") == matchId);
        REQUIRE(view.body.at("host") == "host");
        REQUIRE(view.body.at("phase") == "ACTIVE");
        REQUIRE(view.body.at("revision") == 3);
        REQUIRE(view.body.at("tick") == 0);
        REQUIRE(view.body.at("hiders") == json::array({
            json{{"hider_id", 0}, {"account", "alice"}},
            json{{"hider_id", 1}, {"account", "bob"}}
        }));
    }
}

TEST_CASE("MOVE-002: start with nobody joined is NO_HIDERS", "[hider_seeker][api][lifecycle]") {
    ServiceFixture fx;
    REQUIRE(fx.create("host", 2).status == 201);

    requireError(fx.start("host"), 409, "NO_HIDERS");

    // getMatch lists every slot below hider_count, unbound ones as null.
    const ApiResponse view = fx.getMatch();
    REQUIRE(view.status == 200);
    REQUIRE(view.body.at("phase") == "LOBBY");
    REQUIRE(view.body.at("hiders").size() == 2);
    REQUIRE(view.body.at("hiders").at(0).at("account").is_null());
    REQUIRE(view.body.at("hiders").at(1).at("account").is_null());
}

TEST_CASE("MOVE-002: createMatch validates its body", "[hider_seeker][api][lifecycle]") {
    ServiceFixture fx;
    requireError(fx.service.createMatch(json{{"account", "host"}, {"hider_count", 0}}.dump()), 400, "INVALID_REQUEST");
    requireError(fx.service.createMatch(json{{"account", "host"}, {"hider_count", 9}}.dump()), 400, "INVALID_REQUEST");
    requireError(fx.service.createMatch(json{{"account", "host"}, {"hider_count", 2}, {"seed", -1}}.dump()), 400,
                 "INVALID_REQUEST");
    requireError(fx.service.createMatch(json{{"account", "host"}, {"hider_count", 2}, {"seed", "x"}}.dump()), 400,
                 "INVALID_REQUEST");
    requireError(fx.service.createMatch(json{{"account", "bad name"}, {"hider_count", 2}}.dump()), 400,
                 "INVALID_REQUEST");
    REQUIRE(fx.service.createMatch(json{{"account", "host"}, {"hider_count", 2}}.dump()).status == 201);
}
