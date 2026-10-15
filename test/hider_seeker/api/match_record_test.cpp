#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "state.hpp"
#include "serialization.hpp"
#include "rule_types.hpp"
#include "match_record.hpp"

using namespace hider_seeker;
using nlohmann::json;

namespace {

std::vector<uint8_t> serialize(const MatchState& state) {
    std::vector<uint8_t> out;
    saveMatchState(state, out);
    return out;
}

std::string hexOf(const std::vector<uint8_t>& bytes) {
    static constexpr char HEX_DIGITS[] = "0123456789abcdef";
    std::string hex;
    for (const uint8_t byte : bytes) {
        hex.push_back(HEX_DIGITS[byte >> 4]);
        hex.push_back(HEX_DIGITS[byte & 0x0F]);
    }
    return hex;
}

MatchRecord sampleRecord() {
    MatchRecord record;
    record.filename = "hs-0123456789abcdef";
    record.version = 3;
    record.host = "host";
    record.state.schema_version = 2;
    record.state.ruleset_version = 1;
    record.state.revision = 7;
    record.state.tick = 4;
    record.state.rng_state = 0x1122334455667788ULL;
    record.state.phase = PHASE_ACTIVE;
    record.state.hider_count = 2;
    record.state.server_count = 2;
    record.state.hiders[0].active = 1;
    record.state.hiders[0].action_points = 6;
    record.state.hiders[0].resources[0] = 1;
    record.state.servers[0].active = 1;
    record.state.servers[1].active = 1;
    record.state.servers[1].owner_hider_id = 1;
    record.state.pending[0] = PendingMove{5, 0, 0, 0, NO_SERVER_ID, NO_SERVER_ID, 2};
    record.state.pending_count = 1;
    record.state.next_submit_seq = 1;
    record.accounts.assignValue(0, "alice"); // slot 1 stays unbound
    record.next_move_id = 6;
    record.idempotency.push_back(
        IdempotencyEntry{"alice", "key-1", "move  {}", 200, json{{"status", "APPLIED"}, {"revision", 7}}});
    record.moves.push_back(MoveOwnershipEntry{"mv-5", "alice", 0});
    record.request_log.push_back(RequestLogEntry{0, "host", "create", json{{"account", "host"}}});
    record.request_log.push_back(RequestLogEntry{0, "alice", "join", json{{"account", "alice"}}});
    return record;
}

} // namespace

TEST_CASE("MOVE-001: MatchRecord round-trips through toJson and fromJson", "[hider_seeker][api][match_record]") {
    const MatchRecord original = sampleRecord();
    const json encoded = original.toJson();

    REQUIRE(encoded.at("state") == hexOf(serialize(original.state)));
    REQUIRE(encoded.at("accounts").size() == MAX_HIDERS);
    REQUIRE(encoded.at("accounts").at(0) == "alice");
    REQUIRE(encoded.at("accounts").at(1).is_null());

    MatchRecord decoded;
    REQUIRE(decoded.fromJson(encoded));

    REQUIRE(decoded.filename == original.filename);
    REQUIRE(decoded.version == original.version);
    REQUIRE(decoded.host == original.host);
    REQUIRE(decoded.next_move_id == original.next_move_id);
    REQUIRE(decoded.accounts.getOrDefault(0, decoded.host) == "alice");
    REQUIRE(decoded.accounts.getOrDefault(1, decoded.host).empty());
    REQUIRE(decoded.idempotency.size() == 1);
    REQUIRE(decoded.idempotency[0].status_code == 200);
    REQUIRE(decoded.idempotency[0].response == original.idempotency[0].response);
    REQUIRE(decoded.moves.size() == 1);
    REQUIRE(decoded.moves[0].move_id == "mv-5");
    REQUIRE(decoded.request_log.size() == 2);
    REQUIRE(decoded.request_log[1].kind == "join");

    // The round-tripped state re-serializes byte-for-byte identically.
    REQUIRE(serialize(decoded.state) == serialize(original.state));
    REQUIRE(decoded.toJson() == encoded);
}

TEST_CASE("MOVE-001: fromJson rejects a state field that is not valid hex", "[hider_seeker][api][match_record]") {
    const MatchRecord original = sampleRecord();
    MatchRecord target = sampleRecord();
    target.host = "untouched";

    for (const std::string badState : {"zz", "abc", "0G", "not hex at all"}) {
        json encoded = original.toJson();
        encoded["state"] = badState;
        REQUIRE_FALSE(target.fromJson(encoded));
        REQUIRE(target.host == "untouched");
    }
}

TEST_CASE("MOVE-001: fromJson rejects a schema-version-1 state buffer", "[hider_seeker][api][match_record]") {
    const MatchRecord original = sampleRecord();

    // Hand-built version-1 buffer: the leading little-endian uint32 at byte
    // offset 0 is the buffer layout version saveMatchState writes.
    std::vector<uint8_t> bytes = serialize(original.state);
    REQUIRE(bytes.size() >= 4);
    bytes[0] = 1;
    bytes[1] = 0;
    bytes[2] = 0;
    bytes[3] = 0;

    json encoded = original.toJson();
    encoded["state"] = hexOf(bytes);

    MatchRecord target;
    target.host = "untouched";
    REQUIRE_FALSE(target.fromJson(encoded));
    REQUIRE(target.host == "untouched");
}

TEST_CASE("RECORD-001: public_events round-trips as {type, tick, revision} entries", "[hider_seeker][api][match_record]") {
    MatchRecord original = sampleRecord();
    appendPublicEvent(original, PublicEventEntry{"SERVER_ADDED", 2, 5});
    appendPublicEvent(original, PublicEventEntry{"SERVER_REMOVED", 4, 7});

    const json encoded = original.toJson();
    REQUIRE(encoded.at("public_events") == json::array({
        json{{"type", "SERVER_ADDED"}, {"tick", 2}, {"revision", 5}},
        json{{"type", "SERVER_REMOVED"}, {"tick", 4}, {"revision", 7}}
    }));

    MatchRecord decoded;
    REQUIRE(decoded.fromJson(encoded));
    REQUIRE(decoded.public_events.size() == 2);
    REQUIRE(decoded.public_events[0].type == "SERVER_ADDED");
    REQUIRE(decoded.public_events[0].tick == 2);
    REQUIRE(decoded.public_events[0].revision == 5);
    REQUIRE(decoded.public_events[1].type == "SERVER_REMOVED");
    REQUIRE(decoded.public_events[1].tick == 4);
    REQUIRE(decoded.public_events[1].revision == 7);
    REQUIRE(decoded.toJson() == encoded);
}

TEST_CASE("RECORD-001: a record without public_events loads with an empty list", "[hider_seeker][api][match_record]") {
    json encoded = sampleRecord().toJson();
    encoded.erase("public_events");

    MatchRecord decoded;
    decoded.public_events.push_back(PublicEventEntry{"SERVER_ADDED", 1, 1});
    REQUIRE(decoded.fromJson(encoded));
    REQUIRE(decoded.public_events.empty());
    REQUIRE(decoded.host == "host");
}

TEST_CASE("RECORD-001: fromJson rejects malformed public_events", "[hider_seeker][api][match_record]") {
    const json valid = json{{"type", "SERVER_ADDED"}, {"tick", 1}, {"revision", 2}};
    json extraField = valid;
    extraField["extra"] = true;
    json missingTick = valid;
    missingTick.erase("tick");
    json negativeTick = valid;
    negativeTick["tick"] = -1;
    json stringRevision = valid;
    stringRevision["revision"] = "2";

    const std::vector<json> badValues = {
        json("not an array"),
        json::object(),
        json(nullptr),
        json::array({json{{"type", "SERVER_MOVED"}, {"tick", 1}, {"revision", 2}}}),
        json::array({json{{"type", "server_added"}, {"tick", 1}, {"revision", 2}}}),
        json::array({valid, extraField}),
        json::array({missingTick}),
        json::array({negativeTick}),
        json::array({stringRevision}),
        json::array({json("SERVER_ADDED")})
    };

    for (const json& bad : badValues) {
        json encoded = sampleRecord().toJson();
        encoded["public_events"] = bad;
        MatchRecord target;
        target.host = "untouched";
        REQUIRE_FALSE(target.fromJson(encoded));
        REQUIRE(target.host == "untouched");
    }
}

TEST_CASE("RECORD-001: appendPublicEvent keeps at most MAX_EVENTS, dropping the oldest", "[hider_seeker][api][match_record]") {
    MatchRecord record;
    for (uint32_t i = 0; i < MAX_EVENTS; ++i) {
        appendPublicEvent(record, PublicEventEntry{"SERVER_ADDED", i, i});
    }
    REQUIRE(record.public_events.size() == MAX_EVENTS);
    REQUIRE(record.public_events.front().tick == 0);

    appendPublicEvent(record, PublicEventEntry{"SERVER_REMOVED", 1000, 1000});
    REQUIRE(record.public_events.size() == MAX_EVENTS);
    REQUIRE(record.public_events.front().tick == 1);
    REQUIRE(record.public_events.back().type == "SERVER_REMOVED");
    REQUIRE(record.public_events.back().tick == 1000);

    appendPublicEvent(record, PublicEventEntry{"SERVER_ADDED", 1001, 1001});
    REQUIRE(record.public_events.size() == MAX_EVENTS);
    REQUIRE(record.public_events.front().tick == 2);
    REQUIRE(record.public_events.back().tick == 1001);
}
