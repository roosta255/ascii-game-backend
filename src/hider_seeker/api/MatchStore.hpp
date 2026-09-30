#pragma once

// In-memory registry of hider-seeker matches backing the lobby-management HTTP API
// (create/get/list/join/leave/start). This is the API layer for the domain state
// frozen in src/hider_seeker/domain/state.hpp -- see that file's own header comment
// for what it deliberately still doesn't cover: no tick engine, no intent
// validation/resolution, no NPC AI. Nothing in this class advances `tick` or
// resolves anything; `start()` only flips phase LOBBY -> ACTIVE so a later tick
// engine has somewhere real to pick up from.
//
// Deliberately in-memory only, not persisted to disk -- this slice's own scope
// decision, not something the (currently nonexistent) design doc mandates. Adding
// persistence later is a small, additive change: wrap get()/create()/join()/etc.
// around the already-implemented, already-tested saveMatchState()/loadMatchState()
// instead of holding MatchRecord in a std::unordered_map.
//
// Account/role bookkeeping (who occupies which hider slot, who is the seeker) is
// kept here, in MatchRecord, rather than in MatchState itself: MatchState's own
// struct contract explicitly forbids strings/pointers/growing containers, and
// identity/session bookkeeping was never part of what that struct was frozen to
// hold.

#include "state.hpp"
#include <array>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace hider_seeker {

enum class JoinRole { HIDER, SEEKER };

enum class MatchStoreError {
    NONE,
    NOT_FOUND,
    INVALID_SERVER_COUNT,
    MATCH_NOT_IN_LOBBY,
    MATCH_FULL,
    ACCOUNT_ALREADY_JOINED,
    ACCOUNT_NOT_FOUND,
    NO_HIDERS,
    NO_SEEKER,
};

const char* toString(MatchStoreError error);

struct MatchRecord {
    std::string host;
    MatchState state{};
    std::array<std::string, MAX_HIDERS> hiderAccounts{}; // index parallels state.hiders; "" == unoccupied
    std::string seekerAccount; // "" == unclaimed
};

class MatchStore {
public:
    // Creates a new match in PHASE_LOBBY with `serverCount` servers marked active
    // (serverCount must be in [1, MAX_SERVERS]) and no hiders/seeker joined yet.
    bool create(const std::string& host, uint8_t serverCount, uint32_t rulesetVersion,
                std::string& outMatchId, MatchStoreError& error);

    bool get(const std::string& matchId, MatchRecord& outRecord, MatchStoreError& error) const;

    // Lists up to `limit` match ids starting at `offset`, in creation order.
    // `total` is set to the true total match count regardless of the window.
    void list(int limit, int offset, int& total, std::vector<std::string>& outIds) const;

    // Adds `account` to the match as a hider (next free slot, up to MAX_HIDERS) or as
    // the seeker (exactly one seeker per match). Only valid while phase == LOBBY.
    // An account already occupying any slot in this match cannot join again.
    bool join(const std::string& matchId, const std::string& account, JoinRole role,
              MatchStoreError& error);

    // Removes `account` from whichever slot it occupies. Only valid while
    // phase == LOBBY (once ACTIVE there is no tick engine yet to handle a mid-match
    // departure). Removing a hider compacts the remaining hiders to stay contiguous
    // in [0, hider_count).
    bool leave(const std::string& matchId, const std::string& account, MatchStoreError& error);

    // Transitions LOBBY -> ACTIVE. Requires at least one hider and a claimed seeker.
    bool start(const std::string& matchId, MatchStoreError& error);

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, MatchRecord> matches_;
    std::vector<std::string> order_;
    uint64_t nextId_ = 1;
};

} // namespace hider_seeker
