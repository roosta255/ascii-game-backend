#include <catch2/catch_test_macros.hpp>
#include <string>
#include "state.hpp"
#include "loa_rules.hpp"
#include "service_test_support.hpp"

using namespace hs_test;
using hider_seeker::LOA_RESOURCE_INDEX;

TEST_CASE("MOVE-012b: verify API cancellation reason reporting", "[hider_seeker][api][cancel_reasons]") {
    ServiceFixture fx;
    fx.setupTwoHiders();

    // alice plays ADD_SERVER (should succeed)
    const ApiResponse played = fx.play("alice", intentOf("ADD_SERVER"));
    REQUIRE(played.status == 200);
    REQUIRE(played.body.at("pending_cancelled_reasons").is_array());
    REQUIRE(played.body.at("pending_cancelled_reasons").size() == 0);

    // alice queues PAY_GOAL (insufficient AP, thus QUEUED)
    const ApiResponse queued = fx.queue("alice", intentOf("PAY_GOAL"));
    REQUIRE(queued.status == 200);
    REQUIRE(queued.body.at("status") == "QUEUED");
    const std::string payGoalId = queued.body.at("intent_id").get<std::string>();

    // alice cancels her move
    const ApiResponse cancelled = fx.cancel("alice", payGoalId);
    REQUIRE(cancelled.status == 200);
    REQUIRE(cancelled.body.at("status") == "CANCELLED");
    REQUIRE(cancelled.body.at("pending_cancelled_reasons").is_array());
    REQUIRE(cancelled.body.at("pending_cancelled_reasons").size() == 1);
    REQUIRE(cancelled.body.at("pending_cancelled_reasons")[0].at("intent_id") == payGoalId);
    REQUIRE(cancelled.body.at("pending_cancelled_reasons")[0].at("reason") == "OWNER_CANCELLED");

    // alice queues removeServer(1) (her LOA is on ADD_SERVER, so it is QUEUED)
    const ApiResponse removedQueued = fx.queue("alice", removeServer(1));
    REQUIRE(removedQueued.status == 200);
    REQUIRE(removedQueued.body.at("status") == "QUEUED");
    const std::string removeId = removedQueued.body.at("intent_id").get<std::string>();

    // bob plays removeServer(1) (APPLIED)
    const ApiResponse removedPlayed = fx.play("bob", removeServer(1));
    REQUIRE(removedPlayed.status == 200);
    REQUIRE(removedPlayed.body.at("status") == "APPLIED");

    // bob passes 1 tick
    const ApiResponse passed = fx.pass("bob", 1);
    REQUIRE(passed.status == 200);
    REQUIRE(passed.body.at("pending_cancelled").is_array());
    REQUIRE(passed.body.at("pending_cancelled").size() == 1);
    REQUIRE(passed.body.at("pending_cancelled")[0] == removeId);
    REQUIRE(passed.body.at("pending_cancelled_reasons").is_array());
    REQUIRE(passed.body.at("pending_cancelled_reasons").size() == 1);
    REQUIRE(passed.body.at("pending_cancelled_reasons")[0].at("intent_id") == removeId);
    REQUIRE(passed.body.at("pending_cancelled_reasons")[0].at("reason") == "REJECTED_INVALID_TARGET");
}