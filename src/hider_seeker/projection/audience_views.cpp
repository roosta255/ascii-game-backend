#include "audience_views.hpp"
#include "../ruleset/action_rules.hpp"
#include "../ruleset/loa_rules.hpp"
#include "../ruleset/probe_rules.hpp"
#include "../tick/tick_types.hpp"

namespace hider_seeker {

// PROJ-006 non-interference: this file reads only the MatchState fields named
// below. It never reads rng_state, seeker, goals, the events ring
// (events/event_head/event_count), next_submit_seq, any pending submit_seq,
// any hider's goal_progress, or any server's owner_hider_id / trace_strength /
// trace_age / public_flags. The public view additionally never reads any
// hider's action_points, resources, or server_id, nor the pending ring.

namespace {

using nlohmann::json;

const char* phaseName(uint8_t phase) {
    switch (phase) {
        case PHASE_LOBBY:    return "LOBBY";
        case PHASE_ACTIVE:   return "ACTIVE";
        case PHASE_COMPLETE: return "COMPLETE";
    }
    return "UNKNOWN";
}

// Reads only hider.active and hider.status.
const char* hiderStatusName(const HiderState& hider) {
    if (hider.active == 0) {
        return "NOT_JOINED";
    }
    if (hider.status == HIDER_STATUS_ACTIVE) {
        return "ACTIVE";
    }
    if (hider.status == HIDER_STATUS_CAPTURED) {
        return "CAPTURED";
    }
    return "UNKNOWN";
}

bool hasServerPosition(const HiderState& hider) {
    return hider.active != 0 && (hider.status == HIDER_STATUS_ACTIVE || hider.status == HIDER_STATUS_CAPTURED);
}

// The account bound to slot, or null for an unbound (or out-of-range) slot.
json accountJson(const ViewContext& ctx, int slot) {
    json account = nullptr;
    ctx.accounts.accessConst(slot, [&account](const std::string& bound) {
        if (!bound.empty()) {
            account = bound;
        }
    });
    return account;
}

bool isBound(const ViewContext& ctx, int slot) {
    return !accountJson(ctx, slot).is_null();
}

const char* actionModeName(uint8_t mode) {
    switch (static_cast<HiderActionMode>(mode)) {
        case HiderActionMode::ADD_SERVER:       return "ADD_SERVER";
        case HiderActionMode::REMOVE_SERVER:    return "REMOVE_SERVER";
        case HiderActionMode::CONFIGURE_SERVER: return "CONFIGURE_SERVER";
        case HiderActionMode::SUPPRESS_ERROR:   return "SUPPRESS_ERROR";
        case HiderActionMode::BOOST_CPU:        return "BOOST_CPU";
        case HiderActionMode::ROTATE_ROUTE:     return "ROTATE_ROUTE";
        case HiderActionMode::PAY_GOAL:         return "PAY_GOAL";
    }
    return "UNKNOWN";
}

// The intent name of a pending move: its TickActionKind for the structural
// kinds, otherwise its HiderActionMode.
const char* pendingActionName(const PendingMove& move) {
    switch (static_cast<TickActionKind>(move.action_type)) {
        case TickActionKind::ADD_SERVER:    return "ADD_SERVER";
        case TickActionKind::REMOVE_SERVER: return "REMOVE_SERVER";
        case TickActionKind::HIDER_ACTION:  break;
    }
    return actionModeName(move.server_mode);
}

json renderMetricFrame(const MetricFrame& frame) {
    json out = json::object();
    out["cpu"] = frame.cpu;
    out["memory"] = frame.memory;
    out["disk"] = frame.disk;
    out["network"] = frame.network;
    out["heat"] = frame.heat;
    out["errors"] = frame.errors;
    out["users"] = frame.users;
    return out;
}

// HISTORY_TICKS frames, oldest first, starting at history_head.
json renderHistory(const ServerState& server) {
    json history = json::array();
    for (uint8_t offset = 0; offset < HISTORY_TICKS; ++offset) {
        history.push_back(renderMetricFrame(server.history[(server.history_head + offset) % HISTORY_TICKS]));
    }
    return history;
}

// Active servers in ascending server_id. Reads only active, current, history,
// and history_head of each server.
json renderServers(const MatchState& state) {
    json servers = json::array();
    // The index is the server_id.
    for (uint8_t serverId = 0; serverId < MAX_SERVERS; ++serverId) {
        const ServerState& server = state.servers[serverId];
        if (server.active == 0) {
            continue;
        }
        servers.push_back({
            {"server_id", serverId},
            {"metrics", renderMetricFrame(server.current)},
            {"history", renderHistory(server)}
        });
    }
    return servers;
}

json renderEvents(const ViewContext& ctx) {
    json events = json::array();
    for (const auto& entry : ctx.publicEvents) {
        events.push_back(toJson(entry));
    }
    return events;
}

// Top-level fields shared by every audience.
json renderCommon(const MatchState& state, const ViewContext& ctx, const char* audience) {
    json view = json::object();
    view["match_id"] = ctx.matchId;
    view["audience"] = audience;
    view["view_version"] = VIEW_VERSION;
    view["schema_version"] = state.schema_version;
    view["ruleset_version"] = state.ruleset_version;
    view["revision"] = state.revision;
    view["tick"] = state.tick;
    view["phase"] = phaseName(state.phase);
    view["hider_count"] = state.hider_count;
    view["server_count"] = state.server_count;
    view["servers"] = renderServers(state);
    view["events"] = renderEvents(ctx);
    return view;
}

// FNV-1a of the compact dump without content_hash, then (re)inserts it.
void computeContentHash(json& view) {
    view.erase("content_hash");
    view["content_hash"] = fnv1a64Hex(view.dump());
}

json renderSelf(const MatchState& state, const ViewContext& ctx, uint8_t hiderId) {
    const HiderState& hider = state.hiders[hiderId];
    json resources = json::array();
    for (const uint16_t amount : hider.resources) {
        resources.push_back(amount);
    }
    return {
        {"hider_id", hiderId},
        {"account", accountJson(ctx, hiderId)},
        {"status", hiderStatusName(hider)},
        {"action_points", hider.action_points},
        {"server_id", hider.server_id},
        {"loa", hider.resources[LOA_RESOURCE_INDEX]},
        {"resources", resources}
    };
}

json renderTeam(const MatchState& state, const ViewContext& ctx, uint8_t hiderId) {
    json team = json::array();
    // The index is the hider_id; only slots below hider_count exist.
    for (uint8_t slot = 0; slot < state.hider_count && slot < MAX_HIDERS; ++slot) {
        if (slot == hiderId) {
            continue;
        }
        const HiderState& hider = state.hiders[slot];
        team.push_back({
            {"hider_id", slot},
            {"account", accountJson(ctx, slot)},
            {"status", hiderStatusName(hider)},
            {"server_id", hasServerPosition(hider) ? json(hider.server_id) : json(nullptr)}
        });
    }
    return team;
}

// The pending ring walked from pending_head in submit order (the same walk
// as the service's pendingMoveIds). Never reads submit_seq.
json renderPending(const MatchState& state, const ViewContext& ctx) {
    json pending = json::array();
    for (uint8_t offset = 0; offset < state.pending_count && offset < MAX_PENDING_MOVES; ++offset) {
        const PendingMove& move = state.pending[(state.pending_head + offset) % MAX_PENDING_MOVES];
        const bool isRemove = static_cast<TickActionKind>(move.action_type) == TickActionKind::REMOVE_SERVER;
        pending.push_back({
            {"intent_id", "mv-" + std::to_string(move.move_id)},
            {"hider_id", move.hider_id},
            {"account", accountJson(ctx, move.hider_id)},
            {"action", pendingActionName(move)},
            {"server_id", isRemove ? json(move.server_id) : json(nullptr)},
            {"queue_position", offset}
        });
    }
    return pending;
}

// Public roster: reads only each hider's active and status.
json renderPublicHiders(const MatchState& state, const ViewContext& ctx) {
    json hiders = json::array();
    // The index is the hider_id; only slots below hider_count exist.
    for (uint8_t slot = 0; slot < state.hider_count && slot < MAX_HIDERS; ++slot) {
        const bool bound = isBound(ctx, slot);
        hiders.push_back({
            {"hider_id", slot},
            {"account", accountJson(ctx, slot)},
            {"status", bound ? hiderStatusName(state.hiders[slot]) : "NOT_JOINED"}
        });
    }
    return hiders;
}

} // namespace

std::string fnv1a64Hex(const std::string& bytes) {
    uint64_t hash = 0xcbf29ce484222325ULL;
    for (const char c : bytes) {
        hash ^= static_cast<uint64_t>(static_cast<unsigned char>(c));
        hash *= 0x100000001b3ULL;
    }
    static constexpr char HEX_DIGITS[] = "0123456789abcdef";
    std::string hex(16, '0');
    for (int nibble = 0; nibble < 16; ++nibble) {
        hex[15 - nibble] = HEX_DIGITS[(hash >> (nibble * 4)) & 0x0F];
    }
    return hex;
}

nlohmann::json buildHiderView(const MatchState& state, const ViewContext& ctx, uint8_t hiderId) {
    json view = renderCommon(state, ctx, "hider");
    view["hider_id"] = hiderId;
    view["self"] = renderSelf(state, ctx, hiderId);
    view["team"] = renderTeam(state, ctx, hiderId);
    view["pending"] = renderPending(state, ctx);
    computeContentHash(view);
    return view;
}

nlohmann::json buildPublicView(const MatchState& state, const ViewContext& ctx) {
    json view = renderCommon(state, ctx, "public");
    view["hiders"] = renderPublicHiders(state, ctx);
    computeContentHash(view);
    return view;
}

} // namespace hider_seeker
