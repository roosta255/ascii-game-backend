#include <catch2/catch_test_macros.hpp>
#include <string>
#include "service_test_support.hpp"

using namespace hs_test;

namespace {

// requireError plus: the body is exactly the {"error","message"} base shape.
void requireBaseError(const ApiResponse& response, int status, const std::string& code) {
    requireError(response, status, code);
    REQUIRE(response.body.size() == 2);
}

} // namespace

TEST_CASE("MOVE-007: submitMove validation rules", "[hider_seeker][api][validation]") {
    ServiceFixture fx;
    fx.setupTwoHiders();

    SECTION("unknown intent.type") {
        requireBaseError(fx.play("alice", intentOf("DANCE")), 400, "INVALID_REQUEST");
    }

    SECTION("play_mode PASS with a non-PASS intent") {
        json body = fx.moveBody("alice", "PASS", intentOf("BOOST_CPU"));
        body["ticks"] = 1;
        requireBaseError(fx.submit(body), 400, "INVALID_REQUEST");
    }

    SECTION("ticks 0") {
        requireBaseError(fx.pass("alice", 0), 400, "INVALID_REQUEST");
    }

    SECTION("ticks 17, one past MAX_ADVANCE_TICKS") {
        requireBaseError(fx.pass("alice", 17), 400, "INVALID_REQUEST");
    }

    SECTION("REMOVE_SERVER without server_id") {
        requireBaseError(fx.play("alice", intentOf("REMOVE_SERVER")), 400, "INVALID_REQUEST");
    }

    SECTION("server_id 16") {
        requireBaseError(fx.play("alice", removeServer(16)), 400, "INVALID_REQUEST");
    }

    SECTION("a hider action naming a server other than the occupied one") {
        // alice occupies server 0.
        requireBaseError(fx.play("alice", json{{"type", "BOOST_CPU"}, {"server_id", 1}}), 400, "SERVER_MISMATCH");
    }

    SECTION("CONNECT without server_id") {
        requireBaseError(fx.play("alice", intentOf("CONNECT")), 400, "INVALID_REQUEST");
    }

    SECTION("an account not in the match") {
        requireBaseError(fx.play("mallory", intentOf("BOOST_CPU")), 403, "NOT_IN_MATCH");
    }

    SECTION("a malformed match id") {
        const json body = fx.moveBody("alice", "PLAY", intentOf("BOOST_CPU"));
        requireBaseError(fx.service.submitMove("bad id!", body.dump()), 400, "INVALID_REQUEST");
    }

    SECTION("a well-formed but unknown match id") {
        const json body = fx.moveBody("alice", "PLAY", intentOf("BOOST_CPU"));
        requireBaseError(fx.service.submitMove("hs-0000000000000000", body.dump()), 404, "MATCH_NOT_FOUND");
    }

    SECTION("a non-JSON body") {
        requireBaseError(fx.service.submitMove(fx.matchId, "not json"), 400, "INVALID_REQUEST");
    }

    // None of the rejected requests changed the match.
    // CONN-008c: fixture connects hiders, so revision is now 5 (start + 2 CONNECTs)
    REQUIRE(fx.revision() == 5);
}
