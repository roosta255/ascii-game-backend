// DEF-006: end-to-end check that a hider action's metric effects and the
// tick's METRICS-step drift reach the stored state, history, and views
// through the move API.

#include <catch2/catch_test_macros.hpp>
#include "state.hpp"
#include "service_test_support.hpp"

using namespace hs_test;
using hider_seeker::HISTORY_TICKS;
using hider_seeker::ServerState;

namespace {

// The history frame `back` pushes before the newest (0 == newest).
uint8_t historyCpu(const ServerState& server, int back) {
    const int index = (server.history_head - 1 - back + HISTORY_TICKS * 2) % HISTORY_TICKS;
    return server.history[index].cpu;
}

} // namespace

TEST_CASE("DEF-006: BOOST_CPU is applied and drifted in the same advanced tick", "[hider_seeker][api][metrics]") {
    ServiceFixture fx;
    fx.setupTwoHiders();

    // Alice starts at 0 AP; BOOST_CPU (2 AP) needs one advanced tick, and
    // runTick applies it before its METRICS step drifts the same tick.
    const ApiResponse played = fx.play("alice", intentOf("BOOST_CPU"));
    REQUIRE(played.status == 200);
    REQUIRE(played.body.at("status") == "APPLIED");
    REQUIRE(played.body.at("ticks_advanced") == 1);

    {
        const ServerState server = fx.record().state.servers[0];
        REQUIRE(server.current.cpu == 14);    // 0 + 15 BOOST_CPU - 1 drift
        REQUIRE(server.trace_strength == 3);  // 0 + 5 BOOST_CPU - 2 drift
        REQUIRE(server.trace_age == 1);       // reset to 0 by the action, then + 1 after decay
        REQUIRE(historyCpu(server, 0) == 14); // newest frame is the drifted frame
    }

    const ApiResponse aliceView = fx.service.getLatestView(fx.matchId, "hider", "alice", "");
    REQUIRE(aliceView.status == 200);
    const json& aliceServer = aliceView.body.at("servers").at(0);
    REQUIRE(aliceServer.at("server_id") == 0);
    REQUIRE(aliceServer.at("metrics").at("cpu") == 14);

    // Bob passes 3 ticks: cpu drifts 14 -> 13 -> 12 -> 11.
    const ApiResponse passed = fx.pass("bob", 3);
    REQUIRE(passed.status == 200);
    REQUIRE(passed.body.at("ticks_advanced") == 3);

    const ServerState server = fx.record().state.servers[0];
    REQUIRE(server.current.cpu == 11); // 14 - 3 x 1 drift
    // The last 4 history frames, oldest to newest.
    REQUIRE(historyCpu(server, 3) == 14);
    REQUIRE(historyCpu(server, 2) == 13); // 14 - 1
    REQUIRE(historyCpu(server, 1) == 12); // 13 - 1
    REQUIRE(historyCpu(server, 0) == 11); // 12 - 1
}
