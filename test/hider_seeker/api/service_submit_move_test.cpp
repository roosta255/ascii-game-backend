#include <catch2/catch_test_macros.hpp>
#include "state.hpp"
#include "loa_rules.hpp"
#include "service_test_support.hpp"

using namespace hs_test;
using hider_seeker::LOA_RESOURCE_INDEX;

TEST_CASE("MOVE-003: PLAY applies now or advances ticks until affordable", "[hider_seeker][api][submit_move]") {
    ServiceFixture fx;
    fx.setupTwoHiders();

    SECTION("BOOST_CPU (2 AP) from 0 AP advances one tick") {
        const ApiResponse played = fx.play("alice", intentOf("BOOST_CPU"));
        REQUIRE(played.status == 200);
        REQUIRE(played.body.at("status") == "APPLIED");
        // CONN-008c: fixture connects hiders, so revision is now 5 (start + 2 CONNECTs)
        REQUIRE(played.body.at("accepted_against_revision") == 5);
        REQUIRE(played.body.at("ticks_advanced") == 1);
        REQUIRE(played.body.at("tick") == 1);
        REQUIRE(played.body.at("revision") == 6); // CONN-008c: +2 for the fixture's CONNECTs
        REQUIRE(played.body.at("intent_id") == "mv-1");
        REQUIRE(played.body.at("pending_applied").empty());
        REQUIRE(played.body.at("pending_cancelled").empty());
        REQUIRE(played.body.at("events").empty());
        REQUIRE(fx.record().state.hiders[0].action_points == 0);
    }

    SECTION("PAY_GOAL (10 AP) from 0 AP advances five ticks") {
        const ApiResponse played = fx.play("alice", intentOf("PAY_GOAL"));
        REQUIRE(played.status == 200);
        REQUIRE(played.body.at("status") == "APPLIED");
        REQUIRE(played.body.at("ticks_advanced") == 5);
        REQUIRE(played.body.at("tick") == 5);
        // CONN-008c: fixture connects hiders, so revision is now 5 (start + 2 CONNECTs)
        REQUIRE(played.body.at("revision") == 6); // CONN-008c: +2 for the fixture's CONNECTs
    }

    SECTION("ADD_SERVER applies once with 1 LOA, then is NEVER_AFFORDABLE") {
        const ApiResponse first = fx.play("alice", intentOf("ADD_SERVER"));
        REQUIRE(first.status == 200);
        REQUIRE(first.body.at("status") == "APPLIED");
        REQUIRE(first.body.at("ticks_advanced") == 0);
        REQUIRE(first.body.at("tick") == 0);
        REQUIRE(first.body.at("events").size() == 1);
        REQUIRE(first.body.at("events").at(0).at("type") == "SERVER_ADDED");
        REQUIRE(fx.record().state.hiders[0].resources[LOA_RESOURCE_INDEX] == 0);

        const uint64_t revisionBefore = fx.revision();
        const ApiResponse second = fx.play("alice", intentOf("ADD_SERVER"));
        REQUIRE(second.status == 422);
        REQUIRE(second.body.at("status") == "REJECTED");
        REQUIRE(second.body.at("rule_outcome") == "NEVER_AFFORDABLE");
        REQUIRE(second.body.at("revision") == revisionBefore);
        REQUIRE(fx.revision() == revisionBefore);
    }
}
