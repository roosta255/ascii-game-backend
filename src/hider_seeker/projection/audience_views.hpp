#pragma once

#include "domain/state.hpp"
#include "data/Array.hpp"
#include "projection/public_event.hpp"
#include <nlohmann/json.hpp>

namespace asciigame {
namespace hider_seeker {

constexpr uint32_t VIEW_VERSION = 1;

struct ViewContext {
    std::string matchId;
    Array<std::string, MAX_HIDERS> accounts;
    std::vector<PublicEventEntry> publicEvents;
};

nlohmann::json buildHiderView(const MatchState& state, const ViewContext& ctx, uint8_t hiderId);
nlohmann::json buildPublicView(const MatchState& state, const ViewContext& ctx);
std::string fnv1a64Hex(const std::string& bytes);

} // namespace hider_seeker
} // namespace asciigame