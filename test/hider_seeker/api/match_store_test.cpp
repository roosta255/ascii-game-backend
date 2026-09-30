#include <catch2/catch_test_macros.hpp>
#include "MatchStore.hpp"

using namespace hider_seeker;

namespace {
    MatchStoreError create(MatchStore& store, std::string& outId, uint8_t serverCount = 4) {
        MatchStoreError error = MatchStoreError::NONE;
        store.create("host_1", serverCount, 1, outId, error);
        return error;
    }
}

TEST_CASE("MatchStore::create initializes a fresh lobby match", "[hider_seeker][match_store]") {
    MatchStore store;
    std::string matchId;
    MatchStoreError error = create(store, matchId, 5);
    REQUIRE(error == MatchStoreError::NONE);
    REQUIRE_FALSE(matchId.empty());

    MatchRecord record;
    REQUIRE(store.get(matchId, record, error));
    REQUIRE(record.host == "host_1");
    REQUIRE(record.state.phase == PHASE_LOBBY);
    REQUIRE(record.state.hider_count == 0);
    REQUIRE(record.state.server_count == 5);
    REQUIRE(record.seekerAccount.empty());
    for (uint8_t i = 0; i < 5; ++i) {
        REQUIRE(record.state.servers[i].active == 1);
        REQUIRE(record.state.servers[i].owner_hider_id == 0xFF);
    }
    for (uint8_t i = 5; i < MAX_SERVERS; ++i) {
        REQUIRE(record.state.servers[i].active == 0);
    }
}

TEST_CASE("MatchStore::create rejects out-of-range server counts", "[hider_seeker][match_store]") {
    MatchStore store;
    std::string matchId;
    REQUIRE(create(store, matchId, 0) == MatchStoreError::INVALID_SERVER_COUNT);
    REQUIRE(create(store, matchId, MAX_SERVERS + 1) == MatchStoreError::INVALID_SERVER_COUNT);
}

TEST_CASE("MatchStore::get returns NOT_FOUND for an unknown id", "[hider_seeker][match_store]") {
    MatchStore store;
    MatchRecord record;
    MatchStoreError error = MatchStoreError::NONE;
    REQUIRE_FALSE(store.get("no_such_match", record, error));
    REQUIRE(error == MatchStoreError::NOT_FOUND);
}

TEST_CASE("MatchStore::list paginates in creation order", "[hider_seeker][match_store]") {
    MatchStore store;
    std::string first, second, third;
    REQUIRE(create(store, first) == MatchStoreError::NONE);
    REQUIRE(create(store, second) == MatchStoreError::NONE);
    REQUIRE(create(store, third) == MatchStoreError::NONE);

    int total = 0;
    std::vector<std::string> ids;
    store.list(2, 0, total, ids);
    REQUIRE(total == 3);
    REQUIRE(ids == std::vector<std::string>{first, second});

    store.list(2, 2, total, ids);
    REQUIRE(total == 3);
    REQUIRE(ids == std::vector<std::string>{third});

    store.list(2, 10, total, ids);
    REQUIRE(total == 3);
    REQUIRE(ids.empty());
}

TEST_CASE("MatchStore::join adds hiders and a seeker", "[hider_seeker][match_store]") {
    MatchStore store;
    std::string matchId;
    REQUIRE(create(store, matchId) == MatchStoreError::NONE);

    MatchStoreError error = MatchStoreError::NONE;
    REQUIRE(store.join(matchId, "alice", JoinRole::HIDER, error));
    REQUIRE(store.join(matchId, "bob", JoinRole::HIDER, error));
    REQUIRE(store.join(matchId, "carol", JoinRole::SEEKER, error));

    MatchRecord record;
    REQUIRE(store.get(matchId, record, error));
    REQUIRE(record.state.hider_count == 2);
    REQUIRE(record.hiderAccounts[0] == "alice");
    REQUIRE(record.hiderAccounts[1] == "bob");
    REQUIRE(record.state.hiders[0].active == 1);
    REQUIRE(record.state.hiders[0].server_id == 0xFF);
    REQUIRE(record.seekerAccount == "carol");
}

TEST_CASE("MatchStore::join rejects a duplicate account", "[hider_seeker][match_store]") {
    MatchStore store;
    std::string matchId;
    REQUIRE(create(store, matchId) == MatchStoreError::NONE);

    MatchStoreError error = MatchStoreError::NONE;
    REQUIRE(store.join(matchId, "alice", JoinRole::HIDER, error));
    REQUIRE_FALSE(store.join(matchId, "alice", JoinRole::HIDER, error));
    REQUIRE(error == MatchStoreError::ACCOUNT_ALREADY_JOINED);
    REQUIRE_FALSE(store.join(matchId, "alice", JoinRole::SEEKER, error));
    REQUIRE(error == MatchStoreError::ACCOUNT_ALREADY_JOINED);
}

TEST_CASE("MatchStore::join rejects a full hider roster and a taken seeker slot", "[hider_seeker][match_store]") {
    MatchStore store;
    std::string matchId;
    REQUIRE(create(store, matchId) == MatchStoreError::NONE);

    MatchStoreError error = MatchStoreError::NONE;
    for (int i = 0; i < MAX_HIDERS; ++i) {
        REQUIRE(store.join(matchId, "hider_" + std::to_string(i), JoinRole::HIDER, error));
    }
    REQUIRE_FALSE(store.join(matchId, "one_too_many", JoinRole::HIDER, error));
    REQUIRE(error == MatchStoreError::MATCH_FULL);

    REQUIRE(store.join(matchId, "carol", JoinRole::SEEKER, error));
    REQUIRE_FALSE(store.join(matchId, "dave", JoinRole::SEEKER, error));
    REQUIRE(error == MatchStoreError::MATCH_FULL);
}

TEST_CASE("MatchStore::leave compacts the hider array", "[hider_seeker][match_store]") {
    MatchStore store;
    std::string matchId;
    REQUIRE(create(store, matchId) == MatchStoreError::NONE);

    MatchStoreError error = MatchStoreError::NONE;
    REQUIRE(store.join(matchId, "alice", JoinRole::HIDER, error));
    REQUIRE(store.join(matchId, "bob", JoinRole::HIDER, error));
    REQUIRE(store.join(matchId, "carol", JoinRole::HIDER, error));

    REQUIRE(store.leave(matchId, "bob", error));

    MatchRecord record;
    REQUIRE(store.get(matchId, record, error));
    REQUIRE(record.state.hider_count == 2);
    REQUIRE(record.hiderAccounts[0] == "alice");
    REQUIRE(record.hiderAccounts[1] == "carol");

    // alice can now rejoin under a fresh slot only after leaving; carol staying put
    // proves the compaction actually moved data, not just the count.
    REQUIRE_FALSE(store.leave(matchId, "bob", error));
    REQUIRE(error == MatchStoreError::ACCOUNT_NOT_FOUND);
}

TEST_CASE("MatchStore::leave clears the seeker slot", "[hider_seeker][match_store]") {
    MatchStore store;
    std::string matchId;
    REQUIRE(create(store, matchId) == MatchStoreError::NONE);

    MatchStoreError error = MatchStoreError::NONE;
    REQUIRE(store.join(matchId, "carol", JoinRole::SEEKER, error));
    REQUIRE(store.leave(matchId, "carol", error));

    MatchRecord record;
    REQUIRE(store.get(matchId, record, error));
    REQUIRE(record.seekerAccount.empty());

    // The slot is free again for someone else.
    REQUIRE(store.join(matchId, "dave", JoinRole::SEEKER, error));
}

TEST_CASE("MatchStore::start requires a hider and a seeker, then flips the phase", "[hider_seeker][match_store]") {
    MatchStore store;
    std::string matchId;
    REQUIRE(create(store, matchId) == MatchStoreError::NONE);

    MatchStoreError error = MatchStoreError::NONE;
    REQUIRE_FALSE(store.start(matchId, error));
    REQUIRE(error == MatchStoreError::NO_HIDERS);

    REQUIRE(store.join(matchId, "alice", JoinRole::HIDER, error));
    REQUIRE_FALSE(store.start(matchId, error));
    REQUIRE(error == MatchStoreError::NO_SEEKER);

    REQUIRE(store.join(matchId, "carol", JoinRole::SEEKER, error));
    REQUIRE(store.start(matchId, error));

    MatchRecord record;
    REQUIRE(store.get(matchId, record, error));
    REQUIRE(record.state.phase == PHASE_ACTIVE);
}

TEST_CASE("MatchStore rejects join/leave/start once a match is no longer in the lobby", "[hider_seeker][match_store]") {
    MatchStore store;
    std::string matchId;
    REQUIRE(create(store, matchId) == MatchStoreError::NONE);

    MatchStoreError error = MatchStoreError::NONE;
    REQUIRE(store.join(matchId, "alice", JoinRole::HIDER, error));
    REQUIRE(store.join(matchId, "carol", JoinRole::SEEKER, error));
    REQUIRE(store.start(matchId, error));

    REQUIRE_FALSE(store.join(matchId, "dave", JoinRole::HIDER, error));
    REQUIRE(error == MatchStoreError::MATCH_NOT_IN_LOBBY);

    REQUIRE_FALSE(store.leave(matchId, "alice", error));
    REQUIRE(error == MatchStoreError::MATCH_NOT_IN_LOBBY);

    REQUIRE_FALSE(store.start(matchId, error));
    REQUIRE(error == MatchStoreError::MATCH_NOT_IN_LOBBY);
}
