#include <catch2/catch_test_macros.hpp>
#include <string>
#include "state.hpp"
#include "loa_rules.hpp"
#include "service_test_support.hpp"

using namespace hs_test;
using hider_seeker::LOA_RESOURCE_INDEX;

TEST_CASE("MOVE-004: QUEUE, PASS, and cancelMove", "[hider_seeker][api][queue_pass_cancel]") {
    ServiceFixture fx;
    fx.setupTwoHiders();

    // alice queues CONFIGURE_SERVER (5 AP) with 0 AP: stored pending, no tick.
    const ApiResponse queued = fx.queue("alice", intentOf("CONFIGURE_SERVER"));
    REQUIRE(queued.status == 200);
    REQUIRE(queued.body.at("status") == "QUEUED");
    REQUIRE(queued.body.at("ticks_advanced") == 0);
    REQUIRE(queued.body.at("tick") == 0);
    const std::string configureId = queued.body.at("intent_id").get<std::string>();

    // bob passes 3 ticks: alice regenerates to 6 AP and her move applies.
    const ApiResponse passed = fx.pass("bob", 3);
    REQUIRE(passed.status == 200);
    REQUIRE(passed.body.at("status") == "APPLIED");
    REQUIRE(passed.body.at("ticks_advanced") == 3);
    REQUIRE(passed.body.at("tick") == 3);
    REQUIRE(passed.body.at("pending_applied") == json::array({configureId}));
    REQUIRE(passed.body.at("pending_cancelled").empty());

    // alice queues PAY_GOAL (10 AP) with 1 AP, then cancels it.
    const ApiResponse payGoal = fx.queue("alice", intentOf("PAY_GOAL"));
    REQUIRE(payGoal.status == 200);
    REQUIRE(payGoal.body.at("status") == "QUEUED");
    const std::string payGoalId = payGoal.body.at("intent_id").get<std::string>();

    // bob cannot cancel alice's move.
    requireError(fx.cancel("bob", payGoalId), 404, "INTENT_NOT_FOUND");

    const ApiResponse cancelled = fx.cancel("alice", payGoalId);
    REQUIRE(cancelled.status == 200);
    REQUIRE(cancelled.body.at("status") == "CANCELLED");
    REQUIRE(cancelled.body.at("ticks_advanced") == 0);
    REQUIRE(cancelled.body.at("pending_cancelled") == json::array({payGoalId}));

    requireError(fx.cancel("alice", payGoalId), 404, "INTENT_NOT_FOUND");
    requireError(fx.cancel("alice", "mv-x"), 400, "INVALID_REQUEST");

    // alice spends her LOA on ADD_SERVER right away...
    const ApiResponse added = fx.play("alice", intentOf("ADD_SERVER"));
    REQUIRE(added.status == 200);
    REQUIRE(added.body.at("status") == "APPLIED");
    REQUIRE(added.body.at("ticks_advanced") == 0);
    REQUIRE(fx.record().state.hiders[0].resources[LOA_RESOURCE_INDEX] == 0);

    // ...so her REMOVE_SERVER of server 1 waits for LOA.
    const ApiResponse removeQueued = fx.queue("alice", removeServer(1));
    REQUIRE(removeQueued.status == 200);
    REQUIRE(removeQueued.body.at("status") == "QUEUED");
    REQUIRE(removeQueued.body.at("ticks_advanced") == 0);
    const std::string removeId = removeQueued.body.at("intent_id").get<std::string>();

    // bob removes server 1 (which he occupies) with his own LOA, now.
    const ApiResponse bobRemoves = fx.play("bob", removeServer(1));
    REQUIRE(bobRemoves.status == 200);
    REQUIRE(bobRemoves.body.at("status") == "APPLIED");
    REQUIRE(bobRemoves.body.at("ticks_advanced") == 0);
    REQUIRE(bobRemoves.body.at("events").at(0).at("type") == "SERVER_REMOVED");
    REQUIRE(fx.record().state.hiders[1].server_id == 0);

    // On the next tick alice's queued move targets an inactive slot and is
    // cancelled rather than applied.
    const ApiResponse nextTick = fx.pass("bob", 1);
    REQUIRE(nextTick.status == 200);
    REQUIRE(nextTick.body.at("pending_applied").empty());
    REQUIRE(nextTick.body.at("pending_cancelled") == json::array({removeId}));

    const auto record = fx.record();
    REQUIRE(record.state.pending_count == 0);
    REQUIRE(record.moves.empty());
}
