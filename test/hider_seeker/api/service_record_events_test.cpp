// RECORD-002: committed moves append their public lifecycle events to
// MatchRecord::public_events; rejected moves and idempotent replays do not.

#include <catch2/catch_test_macros.hpp>
#include <string>
#include "state.hpp"
#include "service_test_support.hpp"

using namespace hs_test;

namespace {

json eventOf(const std::string& type, uint32_t tick, uint64_t revision) {
    return json{{"type", type}, {"tick", tick}, {"revision", revision}};
}

json recordEvents(ServiceFixture& fx) {
    json events = json::array();
    for (const auto& entry : fx.record().public_events) {
        events.push_back(hider_seeker::toJson(entry));
    }
    return events;
}

} // namespace

TEST_CASE("RECORD-002: ADD_SERVER then REMOVE_SERVER append two ordered public events", "[hider_seeker][api][record_events]") {
    ServiceFixture fx;
    fx.setupTwoHiders();
    REQUIRE(fx.record().public_events.empty());

    // alice adds a server at tick 0: revision 4.
    const ApiResponse added = fx.play("alice", intentOf("ADD_SERVER"));
    REQUIRE(added.status == 200);
    REQUIRE(added.body.at("status") == "APPLIED");
    REQUIRE(added.body.at("revision") == 4);
    REQUIRE(added.body.at("events").at(0).at("tick") == 0);

    // bob passes to tick 2 (revision 5), then removes server 1 (revision 6).
    REQUIRE(fx.pass("bob", 2).status == 200);
    const ApiResponse removed = fx.play("bob", removeServer(1));
    REQUIRE(removed.status == 200);
    REQUIRE(removed.body.at("status") == "APPLIED");
    REQUIRE(removed.body.at("revision") == 6);
    REQUIRE(removed.body.at("tick") == 2);
    REQUIRE(removed.body.at("events").at(0).at("tick") == 2);

    const json expected = json::array({eventOf("SERVER_ADDED", 0, 4), eventOf("SERVER_REMOVED", 2, 6)});
    REQUIRE(recordEvents(fx) == expected);

    // Both audiences render the same ordered history.
    const ApiResponse hiderView = fx.service.getLatestView(fx.matchId, "hider", "alice", "");
    REQUIRE(hiderView.status == 200);
    REQUIRE(hiderView.body.at("events") == expected);
    const ApiResponse bobView = fx.service.getLatestView(fx.matchId, "hider", "bob", "");
    REQUIRE(bobView.status == 200);
    REQUIRE(bobView.body.at("events") == expected);
    const ApiResponse publicView = fx.service.getLatestView(fx.matchId, "public", "", "");
    REQUIRE(publicView.status == 200);
    REQUIRE(publicView.body.at("events") == expected);
}

TEST_CASE("RECORD-002: a REJECTED move appends nothing", "[hider_seeker][api][record_events]") {
    ServiceFixture fx;
    fx.setupTwoHiders();
    REQUIRE(fx.play("alice", intentOf("ADD_SERVER")).status == 200);
    const json before = recordEvents(fx);
    REQUIRE(before.size() == 1);

    // alice has no LOA left: NEVER_AFFORDABLE.
    const ApiResponse rejected = fx.play("alice", intentOf("ADD_SERVER"));
    REQUIRE(rejected.status == 422);
    REQUIRE(rejected.body.at("status") == "REJECTED");
    REQUIRE(recordEvents(fx) == before);
}

TEST_CASE("RECORD-002: an idempotent replay appends nothing", "[hider_seeker][api][record_events]") {
    ServiceFixture fx;
    fx.setupTwoHiders();

    const json body = fx.moveBody("alice", "PLAY", intentOf("ADD_SERVER"));
    const ApiResponse first = fx.submit(body);
    REQUIRE(first.status == 200);
    REQUIRE(first.body.at("status") == "APPLIED");
    const json after = recordEvents(fx);
    REQUIRE(after == json::array({eventOf("SERVER_ADDED", 0, 4)}));

    const ApiResponse replay = fx.submit(body);
    REQUIRE(replay.status == first.status);
    REQUIRE(replay.body == first.body);
    REQUIRE(recordEvents(fx) == after);
    REQUIRE(fx.revision() == 4);
}
