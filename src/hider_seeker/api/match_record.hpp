#pragma once

#include "../domain/state.hpp"
#include "../../data/Array.hpp"
#include <cstdint>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace hider_seeker {

// One recorded response to an idempotent request: replayed verbatim when the
// same account repeats the same key with the same request.
struct IdempotencyEntry {
    std::string account;
    std::string key;
    std::string request; // canonical request text the key was first used with
    int status_code = 0;
    nlohmann::json response;
};

// Which account (and hider slot) owns a queued move, keyed by its "mv-<n>" id.
struct MoveOwnershipEntry {
    std::string move_id;
    std::string account;
    uint8_t hider_id = 0;
};

// One accepted state-changing request, in the order it was applied.
struct RequestLogEntry {
    uint64_t revision_before = 0;
    std::string account;
    std::string kind; // one of "create", "join", "start", "move", "cancel"
    nlohmann::json body;
};

// The persisted form of one hider-seeker match. In JSON, `state` is the
// lowercase hex encoding of saveMatchState's byte output and each `accounts`
// slot is an account string or null.
struct MatchRecord {
    std::string filename;
    int version = 0;
    std::string host;
    MatchState state{};
    Array<std::string, MAX_HIDERS> accounts; // empty string == unbound slot (JSON null)
    uint32_t next_move_id = 1;
    std::vector<IdempotencyEntry> idempotency;
    std::vector<MoveOwnershipEntry> moves;
    std::vector<RequestLogEntry> request_log;

    nlohmann::json toJson() const;
    // Returns false, leaving this record unchanged, when j is malformed, its
    // state is not valid hex, or loadMatchState rejects the decoded bytes.
    bool fromJson(const nlohmann::json& j);
};

} // namespace hider_seeker