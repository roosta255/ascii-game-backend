#pragma once

#include <cstdint>
#include <string>
#include <nlohmann/json.hpp>

namespace asciigame {
namespace hider_seeker {

struct PublicEventEntry {
    std::string type;
    uint32_t tick;
    uint64_t revision;
};

nlohmann::json toJson(const PublicEventEntry& entry);
bool fromJson(const nlohmann::json& j, PublicEventEntry& entry);

} // namespace hider_seeker
} // namespace asciigame