#include <catch2/catch_test_macros.hpp>
#include "service_test_support.hpp"

using namespace hs_test;

TEST_CASE("MOVE-006: idempotency keys replay, conflict, and are scoped per account",
          "[hider_seeker][api][idempotency]") {
    ServiceFixture fx;
    fx.setupTwoHiders();

    json applied = fx.moveBody("alice", "PLAY", intentOf("BOOST_CPU"));
    applied["idempotency_key"] = "same-key";
    const ApiResponse first = fx.submit(applied);
    REQUIRE(first.status == 200);
    REQUIRE(first.body.at("revision") == 6); // CONN-008c: +2 for the fixture's CONNECTs
    REQUIRE(fx.tick() == 1);

    SECTION("repeating an applied request replays it without changing the match") {
        // Its expected_revision is now stale: the idempotency check runs first.
        const ApiResponse replay = fx.submit(applied);
        REQUIRE(replay.status == first.status);
        REQUIRE(replay.body == first.body);
        REQUIRE(fx.revision() == 6); // CONN-008c: +2 for the fixture's CONNECTs
        REQUIRE(fx.tick() == 1);
        REQUIRE(fx.record().request_log.size() == 7); // CONN-008c: +2 for the fixture's CONNECTs
    }

    SECTION("repeating a REJECTED request replays the identical 422") {
        REQUIRE(fx.play("alice", intentOf("ADD_SERVER")).status == 200);
        json rejected = fx.moveBody("alice", "PLAY", intentOf("ADD_SERVER"));
        rejected["idempotency_key"] = "rejected-key";
        const ApiResponse once = fx.submit(rejected);
        REQUIRE(once.status == 422);
        REQUIRE(once.body.at("status") == "REJECTED");

        const ApiResponse twice = fx.submit(rejected);
        REQUIRE(twice.status == 422);
        REQUIRE(twice.body == once.body);
    }

    SECTION("the same key with a different body is IDEMPOTENCY_KEY_REUSED") {
        json different = fx.moveBody("alice", "PLAY", intentOf("SUPPRESS_ERROR"));
        different["idempotency_key"] = "same-key";
        requireError(fx.submit(different), 422, "IDEMPOTENCY_KEY_REUSED");
        REQUIRE(fx.revision() == 6); // CONN-008c: +2 for the fixture's CONNECTs
    }

    SECTION("the same key from a different account is a separate request") {
        json bobs = fx.moveBody("bob", "PLAY", intentOf("BOOST_CPU"));
        bobs["idempotency_key"] = "same-key";
        const ApiResponse bobResponse = fx.submit(bobs);
        REQUIRE(bobResponse.status == 200);
        REQUIRE(bobResponse.body.at("status") == "APPLIED");
        REQUIRE(bobResponse.body.at("revision") == 7); // CONN-008c: +2 for the fixture's CONNECTs
    }
}
