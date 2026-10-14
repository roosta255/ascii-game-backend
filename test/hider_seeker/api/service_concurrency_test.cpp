#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <string>
#include <thread>
#include <vector>
#include "service_test_support.hpp"

using namespace hs_test;

TEST_CASE("MOVE-005: a stale expected_revision is rejected without touching the record",
          "[hider_seeker][api][concurrency]") {
    ServiceFixture fx;
    fx.setupTwoHiders();
    const std::string storedBefore = fx.temp.readRaw(fx.matchId);

    json body = fx.moveBody("alice", "PLAY", intentOf("BOOST_CPU"));
    body["expected_revision"] = 2;
    const ApiResponse stale = fx.submit(body);

    requireError(stale, 409, "STALE_REVISION");
    REQUIRE(stale.body.at("revision") == 3);
    REQUIRE(fx.temp.readRaw(fx.matchId) == storedBefore);
}

TEST_CASE("MOVE-005: concurrent moves against one revision resolve to one 200 and one 409",
          "[hider_seeker][api][concurrency]") {
    ServiceFixture fx;
    fx.setupTwoHiders();

    const std::string aliceBody = fx.moveBody("alice", "PLAY", intentOf("BOOST_CPU")).dump();
    const std::string bobBody = fx.moveBody("bob", "PLAY", intentOf("BOOST_CPU")).dump();

    int aliceStatus = 0;
    int bobStatus = 0;
    std::thread aliceThread([&] { aliceStatus = fx.service.submitMove(fx.matchId, aliceBody).status; });
    std::thread bobThread([&] { bobStatus = fx.service.submitMove(fx.matchId, bobBody).status; });
    aliceThread.join();
    bobThread.join();

    std::vector<int> statuses{aliceStatus, bobStatus};
    std::sort(statuses.begin(), statuses.end());
    REQUIRE(statuses == std::vector<int>{200, 409});
    REQUIRE(fx.revision() == 4);
}
