#pragma once

// Shared fixture for the HiderSeekerService tests: a service backed by a
// FileStore in a fresh temporary directory, plus request builders.

#include <catch2/catch_test_macros.hpp>
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <nlohmann/json.hpp>
#include "FileStore.hpp"
#include "HiderSeekerService.hpp"
#include "match_record.hpp"

namespace hs_test {

using nlohmann::json;
using hider_seeker::ApiResponse;
using hider_seeker::HiderSeekerService;
using hider_seeker::MatchRecord;

// A FileStore rooted in a unique temporary directory, removed on destruction.
class TempStore {
public:
    TempStore() : dir_(makeDir()), store_(dir_) {}
    ~TempStore() {
        std::error_code ignored;
        std::filesystem::remove_all(dir_, ignored);
    }
    TempStore(const TempStore&) = delete;
    TempStore& operator=(const TempStore&) = delete;

    FileStore& store() { return store_; }

    // The directory every stored document lives in, as "<dir>/<name>.json".
    const std::string& dir() const { return dir_; }

    // The exact bytes stored for matchId.
    std::string readRaw(const std::string& matchId) const {
        std::ifstream file(dir_ + "/" + matchId + ".json");
        std::stringstream contents;
        contents << file.rdbuf();
        return contents.str();
    }

private:
    static std::string makeDir() {
        static std::atomic<int> counter{0};
        std::random_device device;
        const auto path = std::filesystem::temp_directory_path() /
                          ("hs_api_test_" + std::to_string(device()) + "_" + std::to_string(counter++));
        return path.string();
    }

    std::string dir_;
    FileStore store_;
};

// One service plus one match on it. Every builder fills in the match's
// current revision and a fresh idempotency key, so tests only state intent.
struct ServiceFixture {
    TempStore temp;
    HiderSeekerService service{temp.store()};
    std::string matchId;
    int keyCounter = 0;

    std::string nextKey() { return "key-" + std::to_string(++keyCounter); }

    ApiResponse getMatch() { return service.getMatch(matchId); }
    uint64_t revision() { return getMatch().body.at("revision").get<uint64_t>(); }
    uint32_t tick() { return getMatch().body.at("tick").get<uint32_t>(); }

    MatchRecord record() {
        MatchRecord loaded;
        REQUIRE(loaded.fromJson(json::parse(temp.readRaw(matchId))));
        return loaded;
    }

    ApiResponse create(const std::string& host, int hiderCount, uint64_t seed = 42) {
        const ApiResponse response =
            service.createMatch(json{{"account", host}, {"hider_count", hiderCount}, {"seed", seed}}.dump());
        if (response.status == 201) {
            matchId = response.body.at("match_id").get<std::string>();
        }
        return response;
    }

    json guarded(const std::string& account) {
        return json{{"account", account}, {"expected_revision", revision()}, {"idempotency_key", nextKey()}};
    }

    ApiResponse join(const std::string& account) { return service.joinMatch(matchId, guarded(account).dump()); }
    ApiResponse start(const std::string& account) { return service.startMatch(matchId, guarded(account).dump()); }

    json moveBody(const std::string& account, const std::string& playMode, const json& intent) {
        json body = guarded(account);
        body["play_mode"] = playMode;
        body["intent"] = intent;
        return body;
    }

    ApiResponse submit(const json& body) { return service.submitMove(matchId, body.dump()); }

    ApiResponse play(const std::string& account, const json& intent) {
        return submit(moveBody(account, "PLAY", intent));
    }
    ApiResponse queue(const std::string& account, const json& intent) {
        return submit(moveBody(account, "QUEUE", intent));
    }
    ApiResponse pass(const std::string& account, int ticks) {
        json body = moveBody(account, "PASS", json{{"type", "PASS"}});
        body["ticks"] = ticks;
        return submit(body);
    }
    ApiResponse cancel(const std::string& account, const std::string& intentId) {
        return service.cancelMove(matchId, intentId, guarded(account).dump());
    }

    // host creates a 2-hider match, alice (slot 0) and bob (slot 1) join, host
    // starts it: revision 3, tick 0, both hiders disconnected, pool 6.
    void setupTwoHidersDisconnected(uint64_t seed = 42) {
        REQUIRE(create("host", 2, seed).status == 201);
        REQUIRE(join("alice").status == 200);
        REQUIRE(join("bob").status == 200);
        REQUIRE(start("host").status == 200);
        REQUIRE(revision() == 3);
    }

    // setupTwoHidersDisconnected, then alice CONNECTs to server 0 and bob to
    // server 1 (CONN-008c): revision 3 + 2 CONNECTs = 5, tick 0, pool 4.
    void setupTwoHiders(uint64_t seed = 42) {
        setupTwoHidersDisconnected(seed);
        REQUIRE(play("alice", json{{"type", "CONNECT"}, {"server_id", 0}}).status == 200);
        REQUIRE(play("bob", json{{"type", "CONNECT"}, {"server_id", 1}}).status == 200);
        REQUIRE(revision() == 5);
    }
};

inline json intentOf(const std::string& type) { return json{{"type", type}}; }

inline json removeServer(int serverId) { return json{{"type", "REMOVE_SERVER"}, {"server_id", serverId}}; }

// The {"error","message"} base shape with the given error code.
inline void requireError(const ApiResponse& response, int status, const std::string& code) {
    REQUIRE(response.status == status);
    REQUIRE(response.body.is_object());
    REQUIRE(response.body.at("error") == code);
    REQUIRE(response.body.at("message").is_string());
}

} // namespace hs_test
