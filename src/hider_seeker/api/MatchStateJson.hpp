#pragma once

// Read-only JSON projection of a hider-seeker MatchRecord for the HTTP API. This is
// separate from src/hider_seeker/domain/serialization.{hpp,cpp}'s own binary,
// versioned save/load: that pair is for persistence (and explicitly must never be a
// raw memory dump); this is for handing a snapshot to an HTTP client, which is a
// different concern with different rules -- plain, human-readable JSON is exactly
// right here.
//
// Fixed-capacity arrays (hiders[MAX_HIDERS], servers[MAX_SERVERS]) are trimmed to
// their live counts (hider_count/server_count) rather than dumping unused slots --
// MAX_HIDERS/MAX_SERVERS are internal capacity limits, not something an API
// consumer should have to filter out itself.

#include "MatchStore.hpp"
#include <json/json.h>
#include <string>

namespace hider_seeker {

Json::Value matchRecordToJson(const std::string& matchId, const MatchRecord& record);

} // namespace hider_seeker
