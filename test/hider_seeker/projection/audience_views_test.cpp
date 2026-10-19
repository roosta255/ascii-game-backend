// PROJ-001..PROJ-006: the audience projection layer, exercised directly from
// MatchState + ViewContext with no MatchRecord, store, or service involved.

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <functional>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <nlohmann/json.hpp>
#include "state.hpp"
#include "rule_types.hpp"
#include "action_rules.hpp"
#include "loa_rules.hpp"
#include "probe_rules.hpp"
#include "lifecycle_events.hpp"
#include "tick_types.hpp"
#include "audience_views.hpp"
#include "match_end.hpp"

using namespace hider_seeker;
using nlohmann::json;

namespace {

MetricFrame frameOf(uint8_t base) {
    return MetricFrame{base, static_cast<uint8_t>(base + 1), static_cast<uint8_t>(base + 2),
                       static_cast<uint8_t>(base + 3), static_cast<uint8_t>(base + 4),
                       static_cast<uint8_t>(base + 5), static_cast<uint8_t>(base + 6)};
}

// An ACTIVE 2-hider match: alice (slot 0) on server 0, bob (slot 1) on
// server 1, both with 1 LOA, with every hidden field holding a non-zero value.
MatchState baseState() {
    MatchState state{};
    state.schema_version = 2;
    state.ruleset_version = 1;
    state.revision = 7;
    state.tick = 3;
    state.rng_state = 0x1122334455667788ULL;
    state.phase = PHASE_ACTIVE;
    state.hider_count = 2;
    state.server_count = 2;
    state.hider_connection_pool = 6; // Initialize the connection pool for 2 hiders

    for (uint8_t slot = 0; slot < 2; ++slot) {
        HiderState& hider = state.hiders[slot];
        hider.active = 1;
        hider.status = HIDER_STATUS_ACTIVE;
        hider.action_points = static_cast<uint8_t>(4 + slot);
        hider.server_id = slot;
        hider.resources[LOA_RESOURCE_INDEX] = 1;
        hider.resources[3] = static_cast<uint16_t>(9 + slot);
        hider.goal_progress[0] = 2;

        ServerState& server = state.servers[slot];
        server.active = 1;
        server.owner_hider_id = slot;
        server.public_flags = 1;
        server.trace_strength = 3;
        server.trace_age = 2;
        server.current = frameOf(static_cast<uint8_t>(10 * (slot + 1)));
    }
    for (uint8_t slot = 2; slot < MAX_HIDERS; ++slot) {
        state.hiders[slot].server_id = NO_SERVER_ID;
    }
    for (uint8_t slot = 2; slot < MAX_SERVERS; ++slot) {
        state.servers[slot].owner_hider_id = 0xFF;
    }

    state.seeker.connection_tokens = 2;
    state.seeker.max_tokens = 8;
    state.seeker.heat = 5;
    state.goals[0].active = 1;
    state.goals[0].goal_id = 4;
    state.events[0] = Event{1, 2};
    state.event_count = 1;
    state.next_submit_seq = 0;
    return state;
}

ViewContext baseContext() {
    ViewContext ctx;
    ctx.matchId = "hs-0123456789abcdef";
    ctx.accounts.assignValue(0, "alice");
    ctx.accounts.assignValue(1, "bob");
    ctx.publicEvents.push_back(PublicEventEntry{"SERVER_ADDED", 1, 4});
    return ctx;
}

std::set<std::string> keysOf(const json& object) {
    std::set<std::string> keys;
    for (auto it = object.begin(); it != object.end(); ++it) {
        keys.insert(it.key());
    }
    return keys;
}

// Every object key appearing anywhere in value, at any depth.
void collectKeys(const json& value, std::set<std::string>& out) {
    if (value.is_object()) {
        for (auto it = value.begin(); it != value.end(); ++it) {
            out.insert(it.key());
            collectKeys(it.value(), out);
        }
    } else if (value.is_array()) {
        for (const auto& element : value) {
            collectKeys(element, out);
        }
    }
}

PendingMove pendingMove(uint32_t moveId, uint32_t submitSeq, uint8_t hiderId, TickActionKind kind, uint8_t serverId,
                        HiderActionMode mode) {
    return PendingMove{moveId, submitSeq, hiderId, static_cast<uint8_t>(kind), serverId, NO_SERVER_ID,
                       static_cast<uint8_t>(mode)};
}

const std::set<std::string> COMMON_KEYS = {
    "match_id", "audience", "view_version", "schema_version", "ruleset_version", "revision", "tick", "phase",
    "hider_count", "server_count", "servers", "events", "winner", "content_hash"
};

} // namespace

TEST_CASE("PROJ-002/003: hider and public views have exactly their documented top-level keys", "[hider_seeker][projection]") {
    const MatchState state = baseState();
    const ViewContext ctx = baseContext();

    std::set<std::string> hiderKeys = COMMON_KEYS;
    hiderKeys.insert({"hider_id", "self", "team", "pending", "team_connections"});
    std::set<std::string> publicKeys = COMMON_KEYS;
    publicKeys.insert("hiders");

    const json hider = buildHiderView(state, ctx, 0);
    const json pub = buildPublicView(state, ctx);
    REQUIRE(keysOf(hider) == hiderKeys);
    REQUIRE(keysOf(pub) == publicKeys);

    REQUIRE(hider.at("match_id") == "hs-0123456789abcdef");
    REQUIRE(hider.at("audience") == "hider");
    REQUIRE(pub.at("audience") == "public");
    REQUIRE(hider.at("view_version") == VIEW_VERSION);
    REQUIRE(hider.at("schema_version") == 2);
    REQUIRE(hider.at("ruleset_version") == 1);
    REQUIRE(hider.at("revision") == 7);
    REQUIRE(hider.at("tick") == 3);
    REQUIRE(hider.at("hider_count") == 2);
    REQUIRE(hider.at("server_count") == 2);
    REQUIRE(hider.at("hider_id") == 0);
    REQUIRE(hider.at("events") == json::array({json{{"type", "SERVER_ADDED"}, {"tick", 1}, {"revision", 4}}}));
    REQUIRE(pub.at("events") == hider.at("events"));
}

TEST_CASE("VIEW-010 (views): view_version 2 and winner handling", "[hider_seeker][projection]") {
    MatchState state = baseState();
    ViewContext ctx = baseContext();

    // Test: both views have view_version 2 and winner null on an active match
    const json hider = buildHiderView(state, ctx, 0);
    const json pub = buildPublicView(state, ctx);
    REQUIRE(hider.at("view_version") == 2);
    REQUIRE(pub.at("view_version") == 2);
    REQUIRE(hider.at("winner").is_null());
    REQUIRE(pub.at("winner").is_null());

    // Test: with phase = PHASE_COMPLETE and winner = SEEKER, both views show "winner": "SEEKER"
    state.phase = PHASE_COMPLETE;
    state.winner = static_cast<uint8_t>(MatchWinner::SEEKER);
    const json hiderCompleteSeeker = buildHiderView(state, ctx, 0);
    const json pubCompleteSeeker = buildPublicView(state, ctx);
    REQUIRE(hiderCompleteSeeker.at("phase") == "COMPLETE");
    REQUIRE(pubCompleteSeeker.at("phase") == "COMPLETE");
    REQUIRE(hiderCompleteSeeker.at("winner") == "SEEKER");
    REQUIRE(pubCompleteSeeker.at("winner") == "SEEKER");

    // Test: with winner = HIDERS, both views show "winner": "HIDERS"
    state.winner = static_cast<uint8_t>(MatchWinner::HIDERS);
    const json hiderCompleteHiders = buildHiderView(state, ctx, 0);
    const json pubCompleteHiders = buildPublicView(state, ctx);
    REQUIRE(hiderCompleteHiders.at("winner") == "HIDERS");
    REQUIRE(pubCompleteHiders.at("winner") == "HIDERS");
}

TEST_CASE("VIEW-010 (views): team_connections in hider view", "[hider_seeker][projection]") {
    MatchState state = baseState();
    const ViewContext ctx = baseContext();

    // Test: the hider view's team_connections equals hider_connection_pool (6 for a freshly started 2-hider match)
    const json hider = buildHiderView(state, ctx, 0);
    REQUIRE(hider.at("team_connections") == 6);
    
    // Test: public view does not contain team_connections
    const json pub = buildPublicView(state, ctx);
    REQUIRE(pub.contains("team_connections") == false);
}

TEST_CASE("VIEW-010 (views): server_id null for disconnected hiders", "[hider_seeker][projection]") {
    MatchState state = baseState();
    const ViewContext ctx = baseContext();

    // Test: disconnected hider's self.server_id is null
    state.hiders[0].server_id = NO_SERVER_ID;
    const json hider = buildHiderView(state, ctx, 0);
    REQUIRE(hider.at("self").at("server_id").is_null());
    
    // Test: team member's server_id reflects actual connection status (not null)
    const json team = hider.at("team");
    REQUIRE(team.size() == 1);
    REQUIRE(team.at(0).at("server_id") == 1);  // hider 1 is still connected to server 1

    // Test: connected hider's server_id contains the correct server number
    state.hiders[0].server_id = 0;
    const json hiderConnected = buildHiderView(state, ctx, 0);
    REQUIRE(hiderConnected.at("self").at("server_id") == 0);
    REQUIRE(hiderConnected.at("team").at(0).at("server_id") == 1);
}

TEST_CASE("VIEW-010 (views): SERVER_PROBED entry in public view", "[hider_seeker][projection]") {
    MatchState state = baseState();
    ViewContext ctx = baseContext();

    // Add a SERVER_PROBED event to the context
    PublicEventEntry entry{"SERVER_PROBED", 2, 5, 2, "HIT"};
    ctx.publicEvents.push_back(entry);
    
    const json pub = buildPublicView(state, ctx);
    REQUIRE(pub.at("events").size() == 2);
    REQUIRE(pub.at("events").at(1).at("type") == "SERVER_PROBED");
    REQUIRE(pub.at("events").at(1).at("server_id") == 2);
    REQUIRE(pub.at("events").at(1).at("outcome") == "HIT");
}

TEST_CASE("PROJ-006: hidden fields never change either view (extended for VIEW-010)", "[hider_seeker][projection]") {
    const ViewContext ctx = baseContext();
    const MatchState base = baseState();
    const std::string hiderBefore = buildHiderView(base, ctx, 0).dump();
    const std::string publicBefore = buildPublicView(base, ctx).dump();

    using Mutation = std::function<void(MatchState&)>;
    const std::vector<std::pair<std::string, Mutation>> hidden = {
        {"rng_state", [](MatchState& s) { s.rng_state ^= 0xFFFF; }},
        {"seeker.connection_tokens", [](MatchState& s) { s.seeker.connection_tokens = 7; }},
        {"seeker.max_tokens", [](MatchState& s) { s.seeker.max_tokens = 3; }},
        {"seeker.heat", [](MatchState& s) { s.seeker.heat = 99; }},
        {"seeker.belief", [](MatchState& s) { s.seeker.belief[1] = 200; }},
        {"seeker.target_ids", [](MatchState& s) { s.seeker.target_ids[0] = 1; }},
        {"seeker.target_count", [](MatchState& s) { s.seeker.target_count = 1; }},
        {"seeker.target_head", [](MatchState& s) { s.seeker.target_head = 4; }},
        {"seeker.last_probe_tick", [](MatchState& s) { s.seeker.last_probe_tick[0] = 9; }},
        {"server0.owner_hider_id", [](MatchState& s) { s.servers[0].owner_hider_id = 1; }},
        {"server1.owner_hider_id", [](MatchState& s) { s.servers[1].owner_hider_id = 0xFF; }},
        {"server0.trace_strength", [](MatchState& s) { s.servers[0].trace_strength = 50; }},
        {"server1.trace_strength", [](MatchState& s) { s.servers[1].trace_strength = 51; }},
        {"server0.trace_age", [](MatchState& s) { s.servers[0].trace_age = 9; }},
        {"server1.trace_age", [](MatchState& s) { s.servers[1].trace_age = 8; }},
        {"server0.public_flags", [](MatchState& s) { s.servers[0].public_flags = 0x80; }},
        {"server1.public_flags", [](MatchState& s) { s.servers[1].public_flags = 0x40; }},
        {"goals", [](MatchState& s) { s.goals[1].active = 1; s.goals[0].reward_amount = 77; }},
        {"events ring", [](MatchState& s) { s.events[5] = Event{2, 9}; }},
        {"event_head", [](MatchState& s) { s.event_head = 3; }},
        {"event_count", [](MatchState& s) { s.event_count = 6; }},
        {"next_submit_seq", [](MatchState& s) { s.next_submit_seq = 1234; }},
        {"bob.action_points", [](MatchState& s) { s.hiders[1].action_points = 33; }},
        {"bob.resources", [](MatchState& s) { s.hiders[1].resources[LOA_RESOURCE_INDEX] = 5; s.hiders[1].resources[7] = 3; }},
        {"bob.goal_progress", [](MatchState& s) { s.hiders[1].goal_progress[3] = 9; }},
        {"alice.goal_progress", [](MatchState& s) { s.hiders[0].goal_progress[3] = 9; }},
    };
    for (const auto& mutation : hidden) {
        INFO(mutation.first);
        MatchState changed = base;
        mutation.second(changed);
        REQUIRE(buildHiderView(changed, ctx, 0).dump() == hiderBefore);
        REQUIRE(buildPublicView(changed, ctx).dump() == publicBefore);
    }

    SECTION("pending submit_seq never changes either view") {
        MatchState withPending = base;
        withPending.pending[0] =
            pendingMove(1, 0, 0, TickActionKind::HIDER_ACTION, NO_SERVER_ID, HiderActionMode::BOOST_CPU);
        withPending.pending_count = 1;
        const std::string hiderWithPending = buildHiderView(withPending, ctx, 0).dump();

        MatchState reseq = withPending;
        reseq.pending[0].submit_seq = 99;
        REQUIRE(buildHiderView(reseq, ctx, 0).dump() == hiderWithPending);
        REQUIRE(buildPublicView(reseq, ctx).dump() == publicBefore);
    }

    SECTION("hider-private fields change only the hider view") {
        const std::vector<std::pair<std::string, Mutation>> privateOnly = {
            {"alice.action_points", [](MatchState& s) { s.hiders[0].action_points = 12; }},
            {"alice.resources", [](MatchState& s) { s.hiders[0].resources[5] = 4; }},
            {"alice.server_id", [](MatchState& s) { s.hiders[0].server_id = 1; }},
            {"bob.server_id", [](MatchState& s) { s.hiders[1].server_id = 0; }},
            {"pending", [](MatchState& s) {
                s.pending[0] = pendingMove(1, 0, 1, TickActionKind::HIDER_ACTION, NO_SERVER_ID,
                                           HiderActionMode::PAY_GOAL);
                s.pending_count = 1;
            }},
        };
        for (const auto& mutation : privateOnly) {
            INFO(mutation.first);
            MatchState changed = base;
            mutation.second(changed);
            REQUIRE(buildHiderView(changed, ctx, 0).dump() != hiderBefore);
            REQUIRE(buildPublicView(changed, ctx).dump() == publicBefore);
        }
    }

    SECTION("an active server's current metrics change both views") {
        MatchState changed = base;
        changed.servers[1].current.cpu = 250;
        REQUIRE(buildHiderView(changed, ctx, 0).dump() != hiderBefore);
        REQUIRE(buildPublicView(changed, ctx).dump() != publicBefore);
    }

    SECTION("hider_connection_pool changes only the hider view") {
        MatchState changed = base;
        changed.hider_connection_pool = 7; // Change the pool size
        const std::string hiderAfter = buildHiderView(changed, ctx, 0).dump();
        const std::string pubAfter = buildPublicView(changed, ctx).dump();
        
        // Hider view should change (has team_connections)
        REQUIRE(hiderAfter != hiderBefore);
        // Public view should remain the same
        REQUIRE(pubAfter == publicBefore);
    }
}

TEST_CASE("PROJ-002: phase uses the LOBBY/ACTIVE/COMPLETE spellings", "[hider_seeker][projection]") {
    MatchState state = baseState();
    const ViewContext ctx = baseContext();
    const std::vector<std::pair<uint8_t, std::string>> phases = {
        {PHASE_LOBBY, "LOBBY"}, {PHASE_ACTIVE, "ACTIVE"}, {PHASE_COMPLETE, "COMPLETE"}
    };
    for (const auto& phase : phases) {
        state.phase = phase.first;
        REQUIRE(buildHiderView(state, ctx, 0).at("phase") == phase.second);
        REQUIRE(buildPublicView(state, ctx).at("phase") == phase.second);
    }
}

TEST_CASE("PROJ-002: self, team, and pending shapes", "[hider_seeker][projection]") {
    MatchState state = baseState();
    ViewContext ctx = baseContext();

    SECTION("self holds the bound hider's private data") {
        const json self = buildHiderView(state, ctx, 0).at("self");
        REQUIRE(keysOf(self) == std::set<std::string>{"hider_id", "account", "status", "action_points", "server_id",
                                                      "loa", "resources"});
        REQUIRE(self.at("hider_id") == 0);
        REQUIRE(self.at("account") == "alice");
        REQUIRE(self.at("status") == "ACTIVE");
        REQUIRE(self.at("action_points") == 4);
        REQUIRE(self.at("server_id") == 0);
        REQUIRE(self.at("loa") == 1);
        REQUIRE(self.at("resources") == json::array({1, 0, 0, 9, 0, 0, 0, 0}));
        REQUIRE(self.at("resources").size() == RESOURCE_TYPE_COUNT);

        state.hiders[0].status = HIDER_STATUS_CAPTURED;
        REQUIRE(buildHiderView(state, ctx, 0).at("self").at("status") == "CAPTURED");
    }

    SECTION("team lists every other slot below hider_count") {
        state.hider_count = 3; // slot 2 unbound and not joined
        const json team = buildHiderView(state, ctx, 0).at("team");
        REQUIRE(team == json::array({
            json{{"hider_id", 1}, {"account", "bob"}, {"status", "ACTIVE"}, {"server_id", 1}},
            json{{"hider_id", 2}, {"account", nullptr}, {"status", "NOT_JOINED"}, {"server_id", nullptr}}
        }));

        state.hiders[1].status = HIDER_STATUS_CAPTURED;
        const json bobView = buildHiderView(state, ctx, 1);
        REQUIRE(bobView.at("team").at(0) ==
                json{{"hider_id", 0}, {"account", "alice"}, {"status", "ACTIVE"}, {"server_id", 0}});
        REQUIRE(buildHiderView(state, ctx, 0).at("team").at(0).at("status") == "CAPTURED");
        REQUIRE(buildHiderView(state, ctx, 0).at("team").at(0).at("server_id") == 1);
    }

    SECTION("alice CONFIGURE_SERVER then bob PAY_GOAL queue in submit order, across the ring wrap") {
        state.pending_head = MAX_PENDING_MOVES - 1;
        state.pending[MAX_PENDING_MOVES - 1] =
            pendingMove(1, 0, 0, TickActionKind::HIDER_ACTION, NO_SERVER_ID, HiderActionMode::CONFIGURE_SERVER);
        state.pending[0] =
            pendingMove(2, 1, 1, TickActionKind::HIDER_ACTION, NO_SERVER_ID, HiderActionMode::PAY_GOAL);
        state.pending_count = 2;
        state.next_submit_seq = 2;

        const json pending = buildHiderView(state, ctx, 0).at("pending");
        REQUIRE(pending == json::array({
            json{{"intent_id", "mv-1"}, {"hider_id", 0}, {"account", "alice"}, {"action", "CONFIGURE_SERVER"},
                 {"server_id", nullptr}, {"queue_position", 0}},
            json{{"intent_id", "mv-2"}, {"hider_id", 1}, {"account", "bob"}, {"action", "PAY_GOAL"},
                 {"server_id", nullptr}, {"queue_position", 1}}
        }));
        // bob sees the same shared queue.
        REQUIRE(buildHiderView(state, ctx, 1).at("pending") == pending);
    }

    SECTION("structural pending moves name their kind; only REMOVE_SERVER carries server_id") {
        state.pending[0] = pendingMove(3, 0, 0, TickActionKind::ADD_SERVER, NO_SERVER_ID, HiderActionMode::ADD_SERVER);
        state.pending[1] = pendingMove(4, 1, 1, TickActionKind::REMOVE_SERVER, 0, HiderActionMode::REMOVE_SERVER);
        state.pending_count = 2;

        const json pending = buildHiderView(state, ctx, 0).at("pending");
        REQUIRE(pending.size() == 2);
        REQUIRE(pending.at(0).at("action") == "ADD_SERVER");
        REQUIRE(pending.at(0).at("server_id").is_null());
        REQUIRE(pending.at(1).at("action") == "REMOVE_SERVER");
        REQUIRE(pending.at(1).at("server_id") == 0);
    }

    SECTION("bob removing server 1 via LOA relocates him to server 0 and drops server 1") {
        Array<PublicLifecycleEvent, MAX_TICK_PUBLIC_EVENTS> events;
        uint8_t eventCount = 0;
        REQUIRE(applyRemoveServer(state, 1, 1, events, eventCount) == RuleOutcome::ACCEPTED);
        REQUIRE(state.hiders[1].server_id == 0);

        const json aliceView = buildHiderView(state, ctx, 0);
        REQUIRE(aliceView.at("team").at(0).at("server_id") == 0);
        REQUIRE(aliceView.at("servers").size() == 1);
        REQUIRE(aliceView.at("servers").at(0).at("server_id") == 0);
        REQUIRE(buildPublicView(state, ctx).at("servers") == aliceView.at("servers"));
        REQUIRE(buildHiderView(state, ctx, 1).at("self").at("server_id") == 0);
    }
}

TEST_CASE("PROJ-003: the public view lists hiders without any private or seeker data", "[hider_seeker][projection]") {
    MatchState state = baseState();
    state.hider_count = 3;
    state.pending[0] = pendingMove(1, 0, 0, TickActionKind::HIDER_ACTION, NO_SERVER_ID, HiderActionMode::BOOST_CPU);
    state.pending_count = 1;
    const ViewContext ctx = baseContext();

    const json pub = buildPublicView(state, ctx);
    REQUIRE(pub.at("hiders") == json::array({
        json{{"hider_id", 0}, {"account", "alice"}, {"status", "ACTIVE"}},
        json{{"hider_id", 1}, {"account", "bob"}, {"status", "ACTIVE"}},
        json{{"hider_id", 2}, {"account", nullptr}, {"status", "NOT_JOINED"}}
    }));

    std::set<std::string> allKeys;
    collectKeys(pub, allKeys);
    for (const std::string forbidden : {"self", "team", "pending", "seeker", "action_points",
                                        "resources", "loa"}) {
        REQUIRE(allKeys.count(forbidden) == 0);
    }
    for (const auto& entry : pub.at("hiders")) {
        REQUIRE(entry.count("server_id") == 0);
    }
    REQUIRE(pub.count("hider_id") == 0);
}

TEST_CASE("PROJ-004: servers render metrics and a 16-frame history oldest first", "[hider_seeker][projection]") {
    MatchState state = baseState();
    const ViewContext ctx = baseContext();

    // Three frames written into an empty ring: slots 0..2, head now 3.
    ServerState& server = state.servers[0];
    server.history[0] = frameOf(100);
    server.history[1] = frameOf(110);
    server.history[2] = frameOf(120);
    server.history_head = 3;

    const json servers = buildPublicView(state, ctx).at("servers");
    REQUIRE(servers.size() == 2);
    const json& first = servers.at(0);
    REQUIRE(keysOf(first) == std::set<std::string>{"server_id", "metrics", "history"});
    REQUIRE(first.at("server_id") == 0);
    REQUIRE(first.at("metrics") == json{{"cpu", 10}, {"memory", 11}, {"disk", 12}, {"network", 13}, {"heat", 14},
                                        {"errors", 15}, {"users", 16}});

    const json& history = first.at("history");
    REQUIRE(history.size() == HISTORY_TICKS);
    const json zero = json{{"cpu", 0}, {"memory", 0}, {"disk", 0}, {"network", 0}, {"heat", 0}, {"errors", 0},
                           {"users", 0}};
    for (int i = 0; i < 13; ++i) {
        REQUIRE(history.at(i) == zero);
    }
    REQUIRE(history.at(13).at("cpu") == 100);
    REQUIRE(history.at(14).at("cpu") == 110);
    REQUIRE(history.at(15).at("cpu") == 120);
    REQUIRE(history.at(15).at("users") == 126);

    REQUIRE(servers.at(1).at("server_id") == 1);
    REQUIRE(servers.at(1).at("history").size() == HISTORY_TICKS);
}

TEST_CASE("PROJ-005: fnv1a64Hex test vectors and content_hash determinism", "[hider_seeker][projection]") {
    REQUIRE(fnv1a64Hex("") == "cbf29ce484222325");
    REQUIRE(fnv1a64Hex("a") == "af63dc4c8601ec8c");

    const MatchState state = baseState();
    const ViewContext ctx = baseContext();

    const json hiderA = buildHiderView(state, ctx, 0);
    const json hiderB = buildHiderView(state, ctx, 0);
    REQUIRE(hiderA.dump() == hiderB.dump());
    REQUIRE(hiderA.at("content_hash") == hiderB.at("content_hash"));

    const json pubA = buildPublicView(state, ctx);
    const json pubB = buildPublicView(state, ctx);
    REQUIRE(pubA.dump() == pubB.dump());

    // content_hash is the hash of the compact dump without content_hash.
    json stripped = hiderA;
    stripped.erase("content_hash");
    REQUIRE(hiderA.at("content_hash") == fnv1a64Hex(stripped.dump()));
    json strippedPub = pubA;
    strippedPub.erase("content_hash");
    REQUIRE(pubA.at("content_hash") == fnv1a64Hex(strippedPub.dump()));
    REQUIRE(hiderA.at("content_hash").get<std::string>().size() == 16);
}

TEST_CASE("PROJ-006: hidden fields never change either view", "[hider_seeker][projection]") {
    const ViewContext ctx = baseContext();
    const MatchState base = baseState();
    const std::string hiderBefore = buildHiderView(base, ctx, 0).dump();
    const std::string publicBefore = buildPublicView(base, ctx).dump();

    using Mutation = std::function<void(MatchState&)>;
    const std::vector<std::pair<std::string, Mutation>> hidden = {
        {"rng_state", [](MatchState& s) { s.rng_state ^= 0xFFFF; }},
        {"seeker.connection_tokens", [](MatchState& s) { s.seeker.connection_tokens = 7; }},
        {"seeker.max_tokens", [](MatchState& s) { s.seeker.max_tokens = 3; }},
        {"seeker.heat", [](MatchState& s) { s.seeker.heat = 99; }},
        {"seeker.belief", [](MatchState& s) { s.seeker.belief[1] = 200; }},
        {"seeker.target_ids", [](MatchState& s) { s.seeker.target_ids[0] = 1; }},
        {"seeker.target_count", [](MatchState& s) { s.seeker.target_count = 1; }},
        {"seeker.target_head", [](MatchState& s) { s.seeker.target_head = 4; }},
        {"seeker.last_probe_tick", [](MatchState& s) { s.seeker.last_probe_tick[0] = 9; }},
        {"server0.owner_hider_id", [](MatchState& s) { s.servers[0].owner_hider_id = 1; }},
        {"server1.owner_hider_id", [](MatchState& s) { s.servers[1].owner_hider_id = 0xFF; }},
        {"server0.trace_strength", [](MatchState& s) { s.servers[0].trace_strength = 50; }},
        {"server1.trace_strength", [](MatchState& s) { s.servers[1].trace_strength = 51; }},
        {"server0.trace_age", [](MatchState& s) { s.servers[0].trace_age = 9; }},
        {"server1.trace_age", [](MatchState& s) { s.servers[1].trace_age = 8; }},
        {"server0.public_flags", [](MatchState& s) { s.servers[0].public_flags = 0x80; }},
        {"server1.public_flags", [](MatchState& s) { s.servers[1].public_flags = 0x40; }},
        {"goals", [](MatchState& s) { s.goals[1].active = 1; s.goals[0].reward_amount = 77; }},
        {"events ring", [](MatchState& s) { s.events[5] = Event{2, 9}; }},
        {"event_head", [](MatchState& s) { s.event_head = 3; }},
        {"event_count", [](MatchState& s) { s.event_count = 6; }},
        {"next_submit_seq", [](MatchState& s) { s.next_submit_seq = 1234; }},
        {"bob.action_points", [](MatchState& s) { s.hiders[1].action_points = 33; }},
        {"bob.resources", [](MatchState& s) { s.hiders[1].resources[LOA_RESOURCE_INDEX] = 5; s.hiders[1].resources[7] = 3; }},
        {"bob.goal_progress", [](MatchState& s) { s.hiders[1].goal_progress[3] = 9; }},
        {"alice.goal_progress", [](MatchState& s) { s.hiders[0].goal_progress[3] = 9; }},
    };
    for (const auto& mutation : hidden) {
        INFO(mutation.first);
        MatchState changed = base;
        mutation.second(changed);
        REQUIRE(buildHiderView(changed, ctx, 0).dump() == hiderBefore);
        REQUIRE(buildPublicView(changed, ctx).dump() == publicBefore);
    }

    SECTION("pending submit_seq never changes either view") {
        MatchState withPending = base;
        withPending.pending[0] =
            pendingMove(1, 0, 0, TickActionKind::HIDER_ACTION, NO_SERVER_ID, HiderActionMode::BOOST_CPU);
        withPending.pending_count = 1;
        const std::string hiderWithPending = buildHiderView(withPending, ctx, 0).dump();

        MatchState reseq = withPending;
        reseq.pending[0].submit_seq = 99;
        REQUIRE(buildHiderView(reseq, ctx, 0).dump() == hiderWithPending);
        REQUIRE(buildPublicView(reseq, ctx).dump() == publicBefore);
    }

    SECTION("hider-private fields change only the hider view") {
        const std::vector<std::pair<std::string, Mutation>> privateOnly = {
            {"alice.action_points", [](MatchState& s) { s.hiders[0].action_points = 12; }},
            {"alice.resources", [](MatchState& s) { s.hiders[0].resources[5] = 4; }},
            {"alice.server_id", [](MatchState& s) { s.hiders[0].server_id = 1; }},
            {"bob.server_id", [](MatchState& s) { s.hiders[1].server_id = 0; }},
            {"pending", [](MatchState& s) {
                s.pending[0] = pendingMove(1, 0, 1, TickActionKind::HIDER_ACTION, NO_SERVER_ID,
                                           HiderActionMode::PAY_GOAL);
                s.pending_count = 1;
            }},
        };
        for (const auto& mutation : privateOnly) {
            INFO(mutation.first);
            MatchState changed = base;
            mutation.second(changed);
            REQUIRE(buildHiderView(changed, ctx, 0).dump() != hiderBefore);
            REQUIRE(buildPublicView(changed, ctx).dump() == publicBefore);
        }
    }

    SECTION("an active server's current metrics change both views") {
        MatchState changed = base;
        changed.servers[1].current.cpu = 250;
        REQUIRE(buildHiderView(changed, ctx, 0).dump() != hiderBefore);
        REQUIRE(buildPublicView(changed, ctx).dump() != publicBefore);
    }
}

namespace {

// Every number appearing anywhere in value, at any depth, under any key.
void collectNumbers(const json& value, std::vector<int64_t>& out) {
    if (value.is_object() || value.is_array()) {
        for (const auto& element : value) {
            collectNumbers(element, out);
        }
    } else if (value.is_number_integer()) {
        out.push_back(value.get<int64_t>());
    }
}

bool containsNumber(const json& view, int64_t number) {
    std::vector<int64_t> numbers;
    collectNumbers(view, numbers);
    for (int64_t value : numbers) {
        if (value == number) {
            return true;
        }
    }
    return false;
}

} // namespace

TEST_CASE("VIEW-002: trace raised by a hider action never reaches any view", "[hider_seeker][projection][metrics]") {
    const ViewContext ctx = baseContext();
    MatchState state = baseState();
    state.hiders[0].action_points = 10;
    state.servers[0].trace_strength = 90; // a value no other rendered field holds

    // CONFIGURE_SERVER raises alice's server trace: 90 + 5 = 95.
    REQUIRE(applyHiderAction(state, 0, HiderActionMode::CONFIGURE_SERVER) == RuleOutcome::ACCEPTED);
    REQUIRE(state.servers[0].trace_strength == 95);
    REQUIRE(state.servers[0].trace_age == 0); // reset because trace rose

    const json hiderViews[] = {buildHiderView(state, ctx, 0), buildHiderView(state, ctx, 1)};
    const json publicView = buildPublicView(state, ctx);

    for (const json& view : {hiderViews[0], hiderViews[1], publicView}) {
        // No key names trace, and trace_strength's value appears under no key.
        std::set<std::string> keys;
        collectKeys(view, keys);
        for (const std::string& key : keys) {
            INFO(key);
            REQUIRE(key.find("trace") == std::string::npos);
        }
        REQUIRE_FALSE(containsNumber(view, 95));
    }

    // Non-interference: the views are identical to the same state with the
    // trace fields put back to their pre-action values.
    MatchState untraced = state;
    untraced.servers[0].trace_strength = 90;
    untraced.servers[0].trace_age = 2;
    REQUIRE(buildHiderView(untraced, ctx, 0).dump() == hiderViews[0].dump());
    REQUIRE(buildHiderView(untraced, ctx, 1).dump() == hiderViews[1].dump());
    REQUIRE(buildPublicView(untraced, ctx).dump() == publicView.dump());
}
