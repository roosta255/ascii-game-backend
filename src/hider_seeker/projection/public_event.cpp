#include "projection/public_event.hpp"
#include <nlohmann/json.hpp>

namespace asciigame {
namespace hider_seeker {

nlohmann::json toJson(const PublicEventEntry& entry) {
    return {
        {"type", entry.type},
        {"tick", entry.tick},
        {"revision", entry.revision}
    };
}

bool fromJson(const nlohmann::json& j, PublicEventEntry& entry) {
    if (!j.is_object()) {
        return false;
    }
    
    if (!j.contains("type") || !j["type"].is_string()) {
        return false;
    }
    
    if (!j.contains("tick") || !j["tick"].is_number_integer()) {
        return false;
    }
    
    if (!j.contains("revision") || !j["revision"].is_number_integer()) {
        return false;
    }
    
    const std::string type = j["type"].get<std::string>();
    if (type != "SERVER_ADDED" && type != "SERVER_REMOVED") {
        return false;
    }
    
    entry.type = type;
    entry.tick = j["tick"].get<uint32_t>();
    entry.revision = j["revision"].get<uint64_t>();
    
    return true;
}

} // namespace hider_seeker
} // namespace asciigame