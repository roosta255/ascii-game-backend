#pragma once

#include <cstdint>
#include <string>
#include <nlohmann/json.hpp>

namespace hider_seeker {

// One public server-lifecycle event as recorded in MatchRecord::public_events
// and rendered in every audience view's "events" array. Dependency-free so
// both the api layer and the projection layer can include it.
struct PublicEventEntry {
    std::string type; // "SERVER_ADDED" or "SERVER_REMOVED"
    uint32_t tick = 0;
    uint64_t revision = 0;
};

// Exactly {"type", "tick", "revision"}.
inline nlohmann::json toJson(const PublicEventEntry& entry) {
    return {{"type", entry.type}, {"tick", entry.tick}, {"revision", entry.revision}};
}

// A JSON integer that is not negative.
inline bool isNonNegativeInteger(const nlohmann::json& value) {
    return value.is_number_unsigned() || (value.is_number_integer() && value.get<int64_t>() >= 0);
}

// True, filling out, only when j is an object with exactly the fields "type"
// ("SERVER_ADDED" or "SERVER_REMOVED"), "tick" (uint32), and "revision"
// (uint64). Leaves out unchanged on false.
inline bool fromJson(const nlohmann::json& j, PublicEventEntry& out) {
    if (!j.is_object() || j.size() != 3) {
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
    return true;
}

} // namespace hider_seeker
