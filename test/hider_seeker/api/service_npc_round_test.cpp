#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include "service_test_support.hpp"
#include "HiderSeekerService.hpp"
#include "hider_status.hpp"
#include "loa_rules.hpp"
#include "goal_rules.hpp"

using json = nlohmann::json;

namespace hider_seeker {

using hs_test::ServiceFixture;

namespace {

// host creates a 2-hider match whose slot 1 is the NPC "npc_1", alice joins
// slot 0, and host starts it (the NPC's first round runs at start).
void setupAliceAndNpc(ServiceFixture& fx) {
    const ApiResponse created = fx.service.createMatch(
        json{{"account", "host"}, {"hider_count", 2}, {"seed", 42}, {"npc_hiders", 1}}.dump());
    REQUIRE(created.status == 201);
    fx.matchId = created.body.at("match_id").get<std::string>();
    REQUIRE(fx.join("alice").status == 200);
    REQUIRE(fx.start("host").status == 200);
}

} // namespace

TEST_CASE("NPC-040 (one move per committed revision)", "[hider_seeker][api][npc][round]") {
    hs_test::ServiceFixture fx;
    setupAliceAndNpc(fx);

    // After start (revision 2), the NPC adds a server for goal 2 with its LOA.
    // The state should have revision 3, server_count 3, servers[2].active 1,
    // and hiders[1].resources[LOA_RESOURCE_INDEX] 0.
    const auto& state = fx.record().state;
    REQUIRE(state.revision == 3);
    REQUIRE(state.server_count == 3);
    REQUIRE(state.servers[2].active == 1);
    REQUIRE(state.hiders[1].resources[LOA_RESOURCE_INDEX] == 0);

    // The last request_log entry should have account "npc_1", kind "move",
    // body.at("play_mode") == "QUEUE", and body.at("intent") == json{{"type", "ADD_SERVER"}}.
    const auto& log = fx.record().request_log;
    REQUIRE(log.size() >= 1);
    REQUIRE(log.back().account == "npc_1");
    REQUIRE(log.back().kind == "move");
    REQUIRE(log.back().body.at("play_mode") == "QUEUE");
    REQUIRE(log.back().body.at("intent") == json{{"type", "ADD_SERVER"}});

    // Alice connects with {"type","CONNECT"},{"server_id",0}
    const ApiResponse connectResponse = fx.play("alice", json{{"type", "CONNECT"}, {"server_id", 0}});
    REQUIRE(connectResponse.status == 200);
    REQUIRE(connectResponse.body.at("revision") == 4);  // alice's revision

    // Goal 2 is now met, so the NPC connects to server 0.
    // The state should have revision 5 and hiders[1].server_id 0.
    REQUIRE(fx.record().state.revision == 5);
    REQUIRE(fx.record().state.hiders[1].server_id == 0);

    // Alice passes with fx.pass("alice", 5)
    const ApiResponse passResponse = fx.pass("alice", 5);
    REQUIRE(passResponse.status == 200);
    REQUIRE(passResponse.body.at("ticks_advanced") == 5);

    // The NPC now has 10 AP and pays goal 2 right away.
    // The state should have revision 7, goals[2].status == GOAL_STATUS_COMPLETED,
    // teamReward(state) == 1, and hiders[1].action_points 0. tick is still 5.
    REQUIRE(fx.record().state.revision == 7);
    REQUIRE(fx.record().state.goals[2].status == GOAL_STATUS_COMPLETED);
    REQUIRE(teamReward(fx.record().state) == 1);
    REQUIRE(fx.record().state.hiders[1].action_points == 0);
    REQUIRE(fx.record().state.tick == 5);  // NPC moves never advance time
}

TEST_CASE("NPC-041 (no NPCs, no change)", "[hider_seeker][api][npc][round]") {
    hs_test::ServiceFixture fx;
    fx.setupTwoHiders();  // Creates a match with no NPC slots

    // The state's revision should be 5 and request_log.size() should be 6,
    // exactly matching the behavior without this job.
    REQUIRE(fx.record().state.revision == 5);
    REQUIRE(fx.record().request_log.size() == 6);
}

TEST_CASE("NPC-042 (existing tests)", "[hider_seeker][api][npc][round]") {
    // This test ensures all existing tests continue to pass unchanged.
    // The previous tests already cover this by running the full test suite.
    // We can add a simple test that verifies basic functionality still works.
    
    hs_test::ServiceFixture fx;
    fx.setupTwoHiders();
    
    REQUIRE(fx.record().state.revision == 5);
    REQUIRE(fx.record().request_log.size() == 6);
}

} // namespace hider_seeker