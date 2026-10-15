#pragma once

// Immutable per-revision audience view snapshots stored in the iStore. This is
// where the api layer, the projection layer, and the store meet; projection/
// itself never depends on any of them.
//
// audienceKey is "public" or "hider-<n>" (n = hider slot). A snapshot of
// revision r is stored as "<match_id>.view.<audienceKey>.rev-<r>" with the
// document {"filename": ..., "version": 0, "view": <view json>}.

#include "match_record.hpp"
#include "../projection/audience_views.hpp"
#include "iStore.hpp"
#include <cstdint>
#include <string>
#include <nlohmann/json.hpp>

namespace hider_seeker {

// The projection inputs held by record besides its MatchState.
ViewContext viewContextOf(const MatchRecord& record);

// Writes the public view and the view of every bound hider slot below
// hider_count at record.state.revision, never overwriting an existing
// snapshot. Attempts every write; false if any failed. Never throws.
bool publishSnapshots(iStore& store, const MatchRecord& record);

// Copies the stored view into outView. False, leaving outView untouched, when
// the snapshot does not exist or its document is malformed.
bool loadSnapshot(iStore& store, const std::string& matchId, const std::string& audienceKey, uint64_t revision,
                  nlohmann::json& outView);

// Stores view as the snapshot only if no document exists at its path yet. True
// when the snapshot is now present (written or already there); false when the
// write failed or the path holds an unreadable document. Never throws.
bool writeSnapshotIfAbsent(iStore& store, const std::string& matchId, const std::string& audienceKey,
                           uint64_t revision, const nlohmann::json& view);

} // namespace hider_seeker
