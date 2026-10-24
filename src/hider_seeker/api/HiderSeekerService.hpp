#pragma once

#include "iStore.hpp"
#include "../projection/audience_views.hpp"
#include "view_snapshots.hpp"
#include "../seeker/policy_interface.hpp"
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>
#include <nlohmann/json.hpp>

namespace hider_seeker {

// HTTP status plus JSON body of one hider-seeker API call, plus any extra
// response headers (empty for every endpoint except the view endpoints).
struct ApiResponse {
    int status;
    nlohmann::json body;
    std::vector<std::pair<std::string, std::string>> headers = {};
};

// All hider-seeker move-API behavior: request validation, per-match
// load/change/save against the store, and dispatch into the turn engine.
// Each request that touches a match holds that match's mutex for its whole
// load-change-save sequence; different matches never block each other.
class HiderSeekerService {
public:
    explicit HiderSeekerService(iStore& store);
    HiderSeekerService(iStore& store, const iSeekerPolicy& policy);

    ApiResponse createMatch(const std::string& body);
    ApiResponse joinMatch(const std::string& matchId, const std::string& body);
    ApiResponse startMatch(const std::string& matchId, const std::string& body);
    ApiResponse getMatch(const std::string& matchId);
    ApiResponse submitMove(const std::string& matchId, const std::string& body);
    ApiResponse cancelMove(const std::string& matchId, const std::string& intentId, const std::string& body);
    // audience is "hider" or "public"; account is ignored for "public".
    // Serves the current revision's snapshot, building (and best-effort
    // storing) it when missing. Headers: ETag, Cache-Control: no-cache.
    ApiResponse getLatestView(const std::string& matchId, const std::string& audience, const std::string& account,
                              const std::string& ifNoneMatch);
    // Serves only an already-stored snapshot of revision (never rebuilt).
    // Headers: ETag, Cache-Control: private, max-age=31536000, immutable.
    ApiResponse getRevisionView(const std::string& matchId, const std::string& audience, const std::string& revision,
                                const std::string& account, const std::string& ifNoneMatch);

private:
    // The mutex for matchId, created on first access under matchMutexesGuard_.
    std::mutex& mutexFor(const std::string& matchId);

    // submitMove / cancelMove / startMatch without the NPC round that follows
    // each of them.
    ApiResponse submitMoveOnce(const std::string& matchId, const std::string& body);
    ApiResponse cancelMoveOnce(const std::string& matchId, const std::string& intentId, const std::string& body);
    ApiResponse startMatchOnce(const std::string& matchId, const std::string& body);
    // NPC-001: after a committed revision, each NPC hider (account "npc_<slot>",
    // ascending slot) reads its own hider view and submits at most one move.
    void runNpcRound(const std::string& matchId);

    iStore& store_;
    const iSeekerPolicy& seekerPolicy_;
    std::map<std::string, std::unique_ptr<std::mutex>> matchMutexes_;
    std::mutex matchMutexesGuard_;
};

} // namespace hider_seeker
