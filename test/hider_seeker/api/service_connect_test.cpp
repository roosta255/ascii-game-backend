#include <catch2/catch_test_macros.hpp>
#include "service_test_support.hpp"
#include "../hider_seeker/ruleset/rule_types.hpp"

using namespace hs_test;

TEST_CASE("CONN-003c: CONNECT and DISCONNECT through the move API", "[hider_seeker][api][connect]") {
    ServiceFixture fx;
    fx.setupTwoHidersDisconnected();

    SECTION("alice CONNECT server 0 in PLAY") {
        const ApiResponse connected = fx.play("alice", json{{"type", "CONNECT"}, {"server_id", 0}});
        REQUIRE(connected.status == 200);
        REQUIRE(connected.body.at("status") == "APPLIED");
        REQUIRE(connected.body.at("ticks_advanced") == 0);
        REQUIRE(connected.body.at("revision") == 4); // 3 after start + this CONNECT
        REQUIRE(fx.record().state.hider_connection_pool == 5);
        REQUIRE(fx.record().state.hiders[0].server_id == 0);
    }

    SECTION("after that, alice CONNECT again") {
        const ApiResponse connected = fx.play("alice", json{{"type", "CONNECT"}, {"server_id", 0}});
        REQUIRE(connected.status == 200);
        REQUIRE(connected.body.at("status") == "APPLIED");
        
        // Now try to connect again - should fail
        const ApiResponse secondConnect = fx.play("alice", json{{"type", "CONNECT"}, {"server_id", 1}});
        REQUIRE(secondConnect.status == 422);
        REQUIRE(secondConnect.body.at("rule_outcome") == "REJECTED_INVALID_TARGET");
    }

    SECTION("alice CONNECT then DISCONNECT") {
        const ApiResponse connected = fx.play("alice", json{{"type", "CONNECT"}, {"server_id", 0}});
        REQUIRE(connected.status == 200);
        REQUIRE(connected.body.at("status") == "APPLIED");

        const ApiResponse disconnected = fx.play("alice", json{{"type", "DISCONNECT"}});
        REQUIRE(disconnected.status == 200);
        REQUIRE(disconnected.body.at("status") == "APPLIED");
        REQUIRE(fx.record().state.hider_connection_pool == 6);
        REQUIRE(fx.record().state.hiders[0].server_id == hider_seeker::NO_SERVER_ID);
        
        // Test: after disconnect, Alice's hider view should show server_id as null
        const auto aliceView = fx.service.getLatestView(fx.matchId, "hider", "alice", "");
        REQUIRE(aliceView.status == 200);
        REQUIRE(aliceView.body.at("self").at("server_id").is_null());
        
        // Test: team_connections size should be 6
        REQUIRE(aliceView.body.at("team_connections") == 6);
    }

    SECTION("bob CONNECT server 5 (inactive)") {
        const ApiResponse connected = fx.play("bob", json{{"type", "CONNECT"}, {"server_id", 5}});
        REQUIRE(connected.status == 422);
        REQUIRE(connected.body.at("rule_outcome") == "REJECTED_INVALID_TARGET");
    }

    SECTION("CONNECT without server_id") {
        const ApiResponse connected = fx.play("alice", json{{"type", "CONNECT"}});
        REQUIRE(connected.status == 400);
        REQUIRE(connected.body.at("error") == "INVALID_REQUEST");
    }

    SECTION("CONNECT with the pool at 0") {
        MatchRecord r = fx.record();
        r.state.hider_connection_pool = 0;
        fx.writeRecord(r);
        
        const ApiResponse connected = fx.play("alice", json{{"type", "CONNECT"}, {"server_id", 0}});
        REQUIRE(connected.status == 422);
        REQUIRE(connected.body.at("rule_outcome") == "REJECTED_NO_CONNECTIONS");
        REQUIRE(fx.revision() == 3); // rejected: nothing committed
        REQUIRE(fx.tick() == 0);     // and no tick advanced
    }

    SECTION("alice BOOST_CPU while disconnected") {
        const ApiResponse boosted = fx.play("alice", intentOf("BOOST_CPU"));
        REQUIRE(boosted.status == 422);
        REQUIRE(boosted.body.at("rule_outcome") == "REJECTED_INVALID_TARGET");
        REQUIRE(fx.record().state.hiders[0].action_points == 0); // No AP spent
    }
}