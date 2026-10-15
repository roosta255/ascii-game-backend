#pragma once

#include "match_record.hpp"
#include "projection/audience_views.hpp"
#include "../store/iStore.hpp"
#include <nlohmann/json.hpp>

namespace asciigame {
namespace hider_seeker {

bool publishSnapshots(iStore& store, const MatchRecord& record);
bool loadSnapshot(iStore& store, const std::string& matchId, const std::string& audienceKey, uint64_t revision, nlohmann::json& outView);
bool writeSnapshotIfAbsent(iStore& store, const std::string& matchId, const std::string& audienceKey, uint64_t revision, const nlohmann::json& view);

} // namespace hider_seeker
} // namespace asciigame