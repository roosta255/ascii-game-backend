#include "MatchStore.hpp"
#include "serialization.hpp"
#include <algorithm>
#include <random>

namespace hider_seeker {

const char* toString(MatchStoreError error) {
    switch (error) {
        case MatchStoreError::NONE:                   return "none";
        case MatchStoreError::NOT_FOUND:               return "not_found";
        case MatchStoreError::INVALID_SERVER_COUNT:    return "invalid_server_count";
        case MatchStoreError::MATCH_NOT_IN_LOBBY:      return "match_not_in_lobby";
        case MatchStoreError::MATCH_FULL:              return "match_full";
        case MatchStoreError::ACCOUNT_ALREADY_JOINED:  return "account_already_joined";
        case MatchStoreError::ACCOUNT_NOT_FOUND:       return "account_not_found";
        case MatchStoreError::NO_HIDERS:               return "no_hiders";
        case MatchStoreError::NO_SEEKER:               return "no_seeker";
    }
    return "unknown_error";
}

namespace {
    uint64_t randomRngSeed() {
        std::random_device rd;
        std::mt19937_64 gen(rd());
        return gen();
    }
}

bool MatchStore::create(const std::string& host, uint8_t serverCount, uint32_t rulesetVersion,
                         std::string& outMatchId, MatchStoreError& error) {
    if (serverCount < 1 || serverCount > MAX_SERVERS) {
        error = MatchStoreError::INVALID_SERVER_COUNT;
        return false;
    }

    MatchRecord record;
    record.host = host;
    record.state.schema_version = currentSchemaVersion();
    record.state.ruleset_version = rulesetVersion;
    record.state.revision = 0;
    record.state.tick = 0;
    record.state.rng_state = randomRngSeed();
    record.state.phase = PHASE_LOBBY;
    record.state.hider_count = 0;
    record.state.server_count = serverCount;
    for (uint8_t i = 0; i < serverCount; ++i) {
        record.state.servers[i].active = 1;
        record.state.servers[i].owner_hider_id = 0xFF;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    outMatchId = "hs_match_" + std::to_string(nextId_++);
    matches_.emplace(outMatchId, std::move(record));
    order_.push_back(outMatchId);
    error = MatchStoreError::NONE;
    return true;
}

bool MatchStore::get(const std::string& matchId, MatchRecord& outRecord, MatchStoreError& error) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = matches_.find(matchId);
    if (it == matches_.end()) {
        error = MatchStoreError::NOT_FOUND;
        return false;
    }
    outRecord = it->second;
    error = MatchStoreError::NONE;
    return true;
}

void MatchStore::list(int limit, int offset, int& total, std::vector<std::string>& outIds) const {
    std::lock_guard<std::mutex> lock(mutex_);
    total = static_cast<int>(order_.size());
    outIds.clear();
    if (offset >= total) return;
    const int end = std::min(total, offset + limit);
    for (int i = offset; i < end; ++i) outIds.push_back(order_[i]);
}

bool MatchStore::join(const std::string& matchId, const std::string& account, JoinRole role,
                      MatchStoreError& error) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = matches_.find(matchId);
    if (it == matches_.end()) {
        error = MatchStoreError::NOT_FOUND;
        return false;
    }
    MatchRecord& record = it->second;
    if (record.state.phase != PHASE_LOBBY) {
        error = MatchStoreError::MATCH_NOT_IN_LOBBY;
        return false;
    }

    const bool alreadySeeker = record.seekerAccount == account;
    const bool alreadyHider = std::find(record.hiderAccounts.begin(),
                                         record.hiderAccounts.begin() + record.state.hider_count,
                                         account) != record.hiderAccounts.begin() + record.state.hider_count;
    if (alreadySeeker || alreadyHider) {
        error = MatchStoreError::ACCOUNT_ALREADY_JOINED;
        return false;
    }

    if (role == JoinRole::HIDER) {
        if (record.state.hider_count >= MAX_HIDERS) {
            error = MatchStoreError::MATCH_FULL;
            return false;
        }
        const uint8_t idx = record.state.hider_count;
        record.hiderAccounts[idx] = account;
        record.state.hiders[idx] = HiderState{};
        record.state.hiders[idx].active = 1;
        record.state.hiders[idx].server_id = 0xFF; // not yet occupying a server
        record.state.hider_count = static_cast<uint8_t>(idx + 1);
    } else {
        if (!record.seekerAccount.empty()) {
            error = MatchStoreError::MATCH_FULL;
            return false;
        }
        record.seekerAccount = account;
        record.state.seeker = SeekerState{};
    }

    error = MatchStoreError::NONE;
    return true;
}

bool MatchStore::leave(const std::string& matchId, const std::string& account, MatchStoreError& error) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = matches_.find(matchId);
    if (it == matches_.end()) {
        error = MatchStoreError::NOT_FOUND;
        return false;
    }
    MatchRecord& record = it->second;
    if (record.state.phase != PHASE_LOBBY) {
        error = MatchStoreError::MATCH_NOT_IN_LOBBY;
        return false;
    }

    if (record.seekerAccount == account) {
        record.seekerAccount.clear();
        record.state.seeker = SeekerState{};
        error = MatchStoreError::NONE;
        return true;
    }

    const uint8_t count = record.state.hider_count;
    for (uint8_t idx = 0; idx < count; ++idx) {
        if (record.hiderAccounts[idx] != account) continue;
        for (uint8_t shift = idx; shift + 1 < count; ++shift) {
            record.hiderAccounts[shift] = record.hiderAccounts[shift + 1];
            record.state.hiders[shift] = record.state.hiders[shift + 1];
        }
        record.hiderAccounts[count - 1] = "";
        record.state.hiders[count - 1] = HiderState{};
        record.state.hider_count = static_cast<uint8_t>(count - 1);
        error = MatchStoreError::NONE;
        return true;
    }

    error = MatchStoreError::ACCOUNT_NOT_FOUND;
    return false;
}

bool MatchStore::start(const std::string& matchId, MatchStoreError& error) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = matches_.find(matchId);
    if (it == matches_.end()) {
        error = MatchStoreError::NOT_FOUND;
        return false;
    }
    MatchRecord& record = it->second;
    if (record.state.phase != PHASE_LOBBY) {
        error = MatchStoreError::MATCH_NOT_IN_LOBBY;
        return false;
    }
    if (record.state.hider_count == 0) {
        error = MatchStoreError::NO_HIDERS;
        return false;
    }
    if (record.seekerAccount.empty()) {
        error = MatchStoreError::NO_SEEKER;
        return false;
    }
    record.state.phase = PHASE_ACTIVE;
    error = MatchStoreError::NONE;
    return true;
}

} // namespace hider_seeker
