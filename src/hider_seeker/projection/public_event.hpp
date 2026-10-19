#pragma once

#include <cstdint>
#include <string>
#include <nlohmann/json.hpp>

namespace hider_seeker {

// One public server-lifecycle event as recorded in MatchRecord::public_events
// and rendered in every audience view's "events" array. Dependency-free so
// both the api layer and the projection layer can include it.
struct PublicEventEntry {
    std::string type; // "SERVER_ADDED" or "SERVER_REMOVED" or "SERVER_PROBED"
    uint32_t tick = 0;
    uint64_t revision = 0;
    // SERVER_PROBED only: the probed server and "HIT" or "MISS".
    uint8_t server_id = 255; // NO_SERVER_ID
    std::string outcome;
};

// Exactly {"type", "tick", "revision"}.
inline nlohmann::json toJson(const PublicEventEntry& entry) {
    if (entry.type == "SERVER_PROBED") {
        return {{"type", entry.type}, {"tick", entry.tick}, {"revision", entry.revision},
                {"server_id", entry.server_id}, {"outcome", entry.outcome}};
    }
    return {{"type", entry.type}, {"tick", entry.tick}, {"revision", entry.revision}};
}

// A JSON integer that is not negative.
inline bool isNonNegativeInteger(const nlohmann::json& value) {
    return value.is_number_unsigned() || (value.is_number_integer() && value.get<int64_t>() >= 0);
}

// True, filling out, only when j is an object with exactly the fields "type"
// ("SERVER_ADDED" or "SERVER_REMOVED" or "SERVER_PROBED"), "tick" (uint32),
// "revision" (uint64), and for SERVER_PROBED also "server_id" (0-255) and
// "outcome" ("HIT" or "MISS"). Leaves out unchanged on false.
inline bool fromJson(const nlohmann::json& j, PublicEventEntry& out) {
    if (!j.is_object()) {
        return false;
    }
    const auto type = j.find("type");
    const auto tick = j.find("tick");
    const auto revision = j.find("revision");
    if (type == j.end() || tick == j.end() || revision == j.end()) {
        return false;
    }
    if (!type->is_string() || !isNonNegativeInteger(*tick) || !isNonNegativeInteger(*revision)) {
        return false;
    }
    const std::string typeName = type->get<std::string>();
    // Check that we have the right number of fields for this event type
    if (typeName == "SERVER_PROBED") {
        if (j.size() != 5) {
            return false;
        }
        const auto server_id = j.find("server_id");
        const auto outcome = j.find("outcome");
        if (server_id == j.end() || outcome == j.end()) {
            return false;
        }
        if (!server_id->is_number_unsigned() || !outcome->is_string()) {
            return false;
        }
        const uint64_t serverIdValue = server_id->get<uint64_t>();
        if (serverIdValue > 255) {
            return false;
        }
        const std::string outcomeValue = outcome->get<std::string>();
        if (outcomeValue != "HIT" && outcomeValue != "MISS") {
            return false;
        }
        const uint64_t tickValue = tick->get<uint64_t>();
        if (tickValue > UINT32_MAX) {
            return false;
        }
        out.type = typeName;
        out.tick = static_cast<uint32_t>(tickValue);
        out.revision = revision->get<uint64_t>();
        out.server_id = static_cast<uint8_t>(serverIdValue);
        out.outcome = outcomeValue;
    } else {
        if (j.size() != 3) {
            return false;
        }
        if (typeName != "SERVER_ADDED" && typeName != "SERVER_REMOVED") {
            return false;
        }
        const uint64_t tickValue = tick->get<uint64_t>();
        if (tickValue > UINT32_MAX) {
            return false;
        }
        out.type = typeName;
        out.tick = static_cast<uint32_t>(tickValue);
        out.revision = revision->get<uint64_t>();
    }
    return true;
}

} // namespace hider_seeker
