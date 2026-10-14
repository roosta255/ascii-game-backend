#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include "src/hider_seeker/api/match_record.hpp"
#include "src/hider_seeker/domain/serialization.hpp"

using namespace hider_seeker;
using json = nlohmann::json;

TEST_CASE("MatchRecord toJson/fromJson round-trip", "[match_record]") {
    MatchRecord record;
    record.filename = "test.json";
    record.version = 1;
    record.host = "test_account";
    record.state = "0123456789abcdef"; // valid hex string
    
    // Set up accounts array
    record.accounts.resize(MAX_HIDERS);
    for (int i = 0; i < MAX_HIDERS; ++i) {
        if (i % 2 == 0) {
            record.accounts[i] = json("account_" + std::to_string(i));
        } else {
            record.accounts[i] = json(nullptr); // null value
        }
    }
    
    record.next_move_id = 42;
    
    // Set up idempotency array
    IdempotencyEntry entry1;
    entry1.account = "account1";
    entry1.key = "key1";
    entry1.request = "request1";
    entry1.status_code = 200;
    entry1.response = json({{"test", "value"}});
    record.idempotency.push_back(entry1);
    
    // Set up moves array
    MoveOwnershipEntry move1;
    move1.move_id = 100;
    move1.account = "account1";
    move1.hider_id = 0;
    record.moves.push_back(move1);
    
    // Set up request_log array
    RequestLogEntry log1;
    log1.revision_before = 0;
    log1.account = "account1";
    log1.kind = "create";
    log1.body = json({{"test", "value"}});
    record.request_log.push_back(log1);
    
    // Test toJson
    json j = record.toJson();
    
    // Test fromJson
    MatchRecord newRecord;
    REQUIRE(newRecord.fromJson(j));
    
    // Verify all fields match
    REQUIRE(newRecord.filename == record.filename);
    REQUIRE(newRecord.version == record.version);
    REQUIRE(newRecord.host == record.host);
    REQUIRE(newRecord.state == record.state);
    REQUIRE(newRecord.next_move_id == record.next_move_id);
    
    REQUIRE(newRecord.accounts.size() == record.accounts.size());
    for (size_t i = 0; i < newRecord.accounts.size(); ++i) {
        REQUIRE(newRecord.accounts[i] == record.accounts[i]);
    }
    
    REQUIRE(newRecord.idempotency.size() == record.idempotency.size());
    REQUIRE(newRecord.idempotency[0].account == record.idempotency[0].account);
    REQUIRE(newRecord.idempotency[0].key == record.idempotency[0].key);
    REQUIRE(newRecord.idempotency[0].request == record.idempotency[0].request);
    REQUIRE(newRecord.idempotency[0].status_code == record.idempotency[0].status_code);
    REQUIRE(newRecord.idempotency[0].response == record.idempotency[0].response);
    
    REQUIRE(newRecord.moves.size() == record.moves.size());
    REQUIRE(newRecord.moves[0].move_id == record.moves[0].move_id);
    REQUIRE(newRecord.moves[0].account == record.moves[0].account);
    REQUIRE(newRecord.moves[0].hider_id == record.moves[0].hider_id);
    
    REQUIRE(newRecord.request_log.size() == record.request_log.size());
    REQUIRE(newRecord.request_log[0].revision_before == record.request_log[0].revision_before);
    REQUIRE(newRecord.request_log[0].account == record.request_log[0].account);
    REQUIRE(newRecord.request_log[0].kind == record.request_log[0].kind);
    REQUIRE(newRecord.request_log[0].body == record.request_log[0].body);
}

TEST_CASE("MatchRecord fromJson returns false for invalid hex", "[match_record]") {
    MatchRecord record;
    json j;
    j["state"] = "invalid_hex_string"; // Invalid hex
    
    REQUIRE_FALSE(record.fromJson(j));
}

// Note: Schema version 1 test would require creating a hand-built byte buffer
// which is complex to implement correctly here without the actual serialization code.