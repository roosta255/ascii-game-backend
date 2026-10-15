#include "projection/audience_views.hpp"
#include "projection/public_event.hpp"
#include <algorithm>
#include <cstring>

namespace asciigame {
namespace hider_seeker {

namespace {

// FNV-1a 64-bit hash implementation
uint64_t fnv1a64(const std::string& bytes) {
    constexpr uint64_t offsetBasis = 0xcbf29ce484222325ULL;
    constexpr uint64_t prime = 0x100000001b3ULL;
    
    uint64_t hash = offsetBasis;
    for (char c : bytes) {
        hash ^= static_cast<uint64_t>(c);
        hash *= prime;
    }
    return hash;
}

nlohmann::json renderMetricFrame(const ServerMetrics& frame) {
    nlohmann::json result;
    result["cpu"] = frame.cpu;
    result["memory"] = frame.memory;
    result["disk"] = frame.disk;
    result["network"] = frame.network;
    result["heat"] = frame.heat;
    result["errors"] = frame.errors;
    result["users"] = frame.users;
    return result;
}

nlohmann::json renderServers(const MatchState& state) {
    nlohmann::json servers = nlohmann::json::array();
    
    for (int i = 0; i < MAX_SERVERS; ++i) {
        const auto& server = state.servers[i];
        if (server.active != 0) {
            nlohmann::json serverJson;
            serverJson["server_id"] = i;
            
            // Render metrics
            nlohmann::json metrics = renderMetricFrame(server.current);
            serverJson["metrics"] = metrics;
            
            // Render history with exactly HISTORY_TICKS frames
            nlohmann::json history = nlohmann::json::array();
            for (int j = 0; j < HISTORY_TICKS; ++j) {
                int index = (server.history_head + j) % HISTORY_TICKS;
                history.push_back(renderMetricFrame(server.history[index]));
            }
            serverJson["history"] = history;
            
            servers.push_back(serverJson);
        }
    }
    
    return servers;
}

nlohmann::json renderEvents(const std::vector<PublicEventEntry>& publicEvents) {
    nlohmann::json events = nlohmann::json::array();
    for (const auto& event : publicEvents) {
        events.push_back(toJson(event));
    }
    return events;
}

std::string computeContentHash(const nlohmann::json& view) {
    // Create a copy without the content_hash field
    nlohmann::json viewCopy = view;
    viewCopy.erase("content_hash");
    
    // Get compact JSON string representation
    std::string jsonStr = viewCopy.dump();
    
    // Compute FNV-1a 64-bit hash and convert to hex
    uint64_t hash = fnv1a64(jsonStr);
    char hex[17];
    snprintf(hex, sizeof(hex), "%016llx", static_cast<unsigned long long>(hash));
    return std::string(hex);
}

} // anonymous namespace

nlohmann::json buildHiderView(const MatchState& state, const ViewContext& ctx, uint8_t hiderId) {
    nlohmann::json view;
    
    view["match_id"] = ctx.matchId;
    view["audience"] = "hider";
    view["view_version"] = VIEW_VERSION;
    view["schema_version"] = state.schema_version;
    view["ruleset_version"] = state.ruleset_version;
    view["revision"] = state.revision;
    view["tick"] = state.tick;
    view["phase"] = phaseName(state.phase);
    view["hider_count"] = state.hider_count;
    view["server_count"] = state.server_count;
    view["servers"] = renderServers(state);
    view["events"] = renderEvents(ctx.publicEvents);
    
    // Compute content_hash before adding it to the view
    std::string contentHash = computeContentHash(view);
    
    // Add content_hash to the view
    view["content_hash"] = contentHash;
    
    // Build self section
    const auto& hider = state.hiders[hiderId];
    nlohmann::json self;
    self["hider_id"] = hiderId;
    self["account"] = ctx.accounts[hiderId];
    
    if (hider.active != 0) {
        if (hider.status == HIDER_STATUS_ACTIVE) {
            self["status"] = "ACTIVE";
        } else if (hider.status == HIDER_STATUS_CAPTURED) {
            self["status"] = "CAPTURED";
        } else {
            self["status"] = "NOT_JOINED";
        }
    } else {
        self["status"] = "NOT_JOINED";
    }
    
    self["action_points"] = hider.action_points;
    self["server_id"] = hider.server_id;
    self["loa"] = hider.resources[LOA_RESOURCE_INDEX];
    
    // Add all resources
    nlohmann::json resources = nlohmann::json::array();
    for (const auto& resource : hider.resources) {
        resources.push_back(resource);
    }
    self["resources"] = resources;
    
    view["self"] = self;
    
    // Build team section
    nlohmann::json team = nlohmann::json::array();
    for (int i = 0; i < state.hider_count; ++i) {
        if (i != hiderId) {
            const auto& otherHider = state.hiders[i];
            nlohmann::json teamMember;
            teamMember["hider_id"] = i;
            teamMember["account"] = ctx.accounts[i];
            
            if (otherHider.active != 0) {
                if (otherHider.status == HIDER_STATUS_ACTIVE) {
                    teamMember["status"] = "ACTIVE";
                    teamMember["server_id"] = otherHider.server_id;
                } else if (otherHider.status == HIDER_STATUS_CAPTURED) {
                    teamMember["status"] = "CAPTURED";
                    teamMember["server_id"] = otherHider.server_id;
                } else {
                    teamMember["status"] = "NOT_JOINED";
                }
            } else {
                teamMember["status"] = "NOT_JOINED";
            }
            
            team.push_back(teamMember);
        }
    }
    view["team"] = team;
    
    // Build pending section
    nlohmann::json pending = nlohmann::json::array();
    int queuePosition = 0;
    for (int i = state.pending_head; i < state.pending_head + state.pending_count; ++i) {
        int index = i % MAX_PENDING_MOVES;
        const auto& intent = state.pending[index];
        nlohmann::json pendingItem;
        pendingItem["intent_id"] = intent.id;
        pendingItem["hider_id"] = intent.hider_id;
        pendingItem["account"] = ctx.accounts[intent.hider_id];
        
        // Determine action type
        if (intent.type == TICK_ACTION_ADD_SERVER || intent.type == TICK_ACTION_REMOVE_SERVER) {
            pendingItem["action"] = tickActionKindName(intent.type);
        } else {
            pendingItem["action"] = hiderActionModeName(intent.type);
        }
        
        // Set server_id for REMOVE_SERVER actions
        if (intent.type == TICK_ACTION_REMOVE_SERVER) {
            pendingItem["server_id"] = intent.target_server_id;
        } else {
            pendingItem["server_id"] = nullptr;
        }
        
        pendingItem["queue_position"] = queuePosition;
        pending.push_back(pendingItem);
        queuePosition++;
    }
    view["pending"] = pending;
    
    view["hider_id"] = hiderId;
    
    return view;
}

nlohmann::json buildPublicView(const MatchState& state, const ViewContext& ctx) {
    nlohmann::json view;
    
    view["match_id"] = ctx.matchId;
    view["audience"] = "public";
    view["view_version"] = VIEW_VERSION;
    view["schema_version"] = state.schema_version;
    view["ruleset_version"] = state.ruleset_version;
    view["revision"] = state.revision;
    view["tick"] = state.tick;
    view["phase"] = phaseName(state.phase);
    view["hider_count"] = state.hider_count;
    view["server_count"] = state.server_count;
    view["servers"] = renderServers(state);
    view["events"] = renderEvents(ctx.publicEvents);
    
    // Compute content_hash before adding it to the view
    std::string contentHash = computeContentHash(view);
    
    // Add content_hash to the view
    view["content_hash"] = contentHash;
    
    // Build hiders section (no server_id, no action_points, resources, etc.)
    nlohmann::json hiders = nlohmann::json::array();
    for (int i = 0; i < state.hider_count; ++i) {
        const auto& hider = state.hiders[i];
        nlohmann::json hiderEntry;
        hiderEntry["hider_id"] = i;
        hiderEntry["account"] = ctx.accounts[i];
        
        if (hider.active != 0) {
            if (hider.status == HIDER_STATUS_ACTIVE) {
                hiderEntry["status"] = "ACTIVE";
            } else if (hider.status == HIDER_STATUS_CAPTURED) {
                hiderEntry["status"] = "CAPTURED";
            } else {
                hiderEntry["status"] = "NOT_JOINED";
            }
        } else {
            hiderEntry["status"] = "NOT_JOINED";
        }
        
        hiders.push_back(hiderEntry);
    }
    view["hiders"] = hiders;
    
    return view;
}

std::string fnv1a64Hex(const std::string& bytes) {
    uint64_t hash = fnv1a64(bytes);
    char hex[17];
    snprintf(hex, sizeof(hex), "%016llx", static_cast<unsigned long long>(hash));
    return std::string(hex);
}

} // namespace hider_seeker
} // namespace asciigame