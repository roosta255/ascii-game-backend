#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace hider_seeker {

struct IdempotencyEntry {
    std::string account;
    std::string key;
    std::string request;
    int status_code;
    nlohmann::json response;
};

struct MoveOwnershipEntry {
    uint32_t move_id;
    std::string account;
    uint8_t hider_id;
};

struct RequestLogEntry {
    int revision_before;
    std::string account;
    std::string kind; // one of "create", "join", "start", "move", "cancel"
    nlohmann::json body;
};

struct MatchRecord {
    std::string filename;
    int version;
    std::string host;
    std::string state; // lowercase hex string of saveMatchState's byte output
    std::vector<nlohmann::json> accounts; // MAX_HIDERS entries, each an optional account string or null
    uint32_t next_move_id = 1;
    std::vector<IdempotencyEntry> idempotency;
    std::vector<MoveOwnershipEntry> moves;
    std::vector<RequestLogEntry> request_log;

    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& j);
};

} // namespace hider_seeker