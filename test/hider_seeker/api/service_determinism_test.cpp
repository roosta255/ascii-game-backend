#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>
#include "service_test_support.hpp"

using namespace hs_test;

namespace {

// Runs one fixed request sequence (including one REJECTED and one stale
// request) and returns every response body, with match_id stripped.
std::vector<json> runScript(ServiceFixture& fx, uint64_t seed) {
    std::vector<json> bodies;
    const auto keep = [&bodies](ApiResponse response) {
        response.body.erase("match_id");
        bodies.push_back(json{{"status", response.status}, {"body", response.body}});
    };

    keep(fx.create("host", 2, seed));
    keep(fx.join("alice"));
    keep(fx.join("bob"));
    keep(fx.start("host"));
    // CONN-008c: hiders start disconnected, so connect them as the shared fixture does.
    keep(fx.play("alice", json{{"type", "CONNECT"}, {"server_id", 0}}));
    keep(fx.play("bob", json{{"type", "CONNECT"}, {"server_id", 1}}));
    keep(fx.play("alice", intentOf("BOOST_CPU")));
    keep(fx.queue("bob", intentOf("CONFIGURE_SERVER")));
    keep(fx.pass("alice", 2));
    keep(fx.play("alice", intentOf("ADD_SERVER")));
    keep(fx.play("alice", intentOf("ADD_SERVER"))); // REJECTED: no LOA left

    json stale = fx.moveBody("bob", "PLAY", intentOf("BOOST_CPU"));
    stale["expected_revision"] = 0;
    keep(fx.submit(stale));
    return bodies;
}

} // namespace

TEST_CASE("MOVE-008: same seed and requests give identical matches", "[hider_seeker][api][determinism]") {
    ServiceFixture first;
    ServiceFixture second;
    const std::vector<json> firstBodies = runScript(first, 777);
    const std::vector<json> secondBodies = runScript(second, 777);

    REQUIRE(firstBodies == secondBodies);
    // CONN-008c: +2 for the script's CONNECTs.
    REQUIRE(firstBodies[10].at("status") == 422);
    REQUIRE(firstBodies[11].at("status") == 409);
    REQUIRE(first.record().toJson().at("state") == second.record().toJson().at("state"));
}

TEST_CASE("MOVE-008: request_log records accepted state changes in order", "[hider_seeker][api][determinism]") {
    ServiceFixture fx;
    runScript(fx, 777);

    struct Expected {
        uint64_t revisionBefore;
        std::string account;
        std::string kind;
    };
    const std::vector<Expected> expected{
        {0, "host", "create"},
        {0, "alice", "join"},
        {1, "bob", "join"},
        {2, "host", "start"},
        {3, "alice", "move"}, // CONN-008c: alice's CONNECT
        {4, "bob", "move"},   // CONN-008c: bob's CONNECT
        {5, "alice", "move"},
        {6, "bob", "move"},
        {7, "alice", "move"},
        {8, "alice", "move"},
    };

    const auto record = fx.record();
    REQUIRE(record.request_log.size() == expected.size());
    for (size_t i = 0; i < expected.size(); ++i) {
        REQUIRE(record.request_log[i].revision_before == expected[i].revisionBefore);
        REQUIRE(record.request_log[i].account == expected[i].account);
        REQUIRE(record.request_log[i].kind == expected[i].kind);
        REQUIRE(record.request_log[i].body.at("account") == expected[i].account);
    }
    // CONN-008c: +2 for the script's CONNECTs.
    REQUIRE(record.request_log[8].body.at("play_mode") == "PASS");
    REQUIRE(record.state.revision == 9);
}
