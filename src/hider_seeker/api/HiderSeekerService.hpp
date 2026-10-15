#pragma once

#include "iStore.hpp"
#include "projection/audience_views.hpp"
#include "view_snapshots.hpp"
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <nlohmann/json.hpp>
#include <vector>
#include <utility>

namespace hider_seeker {

// HTTP status plus JSON body of one hider-seeker API call.
struct ApiResponse {
    int status;
    nlohmann::json body;
    std::vector<std::pair<std::string, std::string>> headers = {};  // Added headers support
};

// All hider-seeker move-API behavior: request validation, per-match
// load/change/save against the store, and dispatch into the turn engine.
// Each request that touches a match holds that match's mutex for its whole
// load-change-save sequence; different matches never block each other.
class HiderSeekerService {
public:
    explicit HiderSeekerService(iStore& store);

    ApiResponse createMatch(const std::string& body);
    ApiResponse joinMatch(const std::string& body);
    ApiResponse startMatch(const std::string& matchId, const std::string& body);
    ApiResponse getMatch(const std::string& matchId);
    ApiResponse submitMove(const std::string& matchId, const std::string& body);
    ApiResponse cancelMove(const std::string& matchId, const std::string& intentId, const std::string& body);
    ApiResponse getLatestView(const std::string& matchId, const std::string& audience, const std::string& account, const std::string& ifNoneMatch);
    ApiResponse getRevisionView(const std::string& matchId, const std::string& audience, const std::string& revision, const std::string& account, const std::string& ifNoneMatch);

private:
    // The mutex for matchId, created on first access under matchMutexesGuard_.
    std::mutex& mutexFor(const std::string& matchId);

    iStore& store_;
    std::map<std::string, std::unique_ptr<std::mutex>> matchMutexes_;
    std::mutex matchMutexesGuard_;
};

} // namespace hider_seeker