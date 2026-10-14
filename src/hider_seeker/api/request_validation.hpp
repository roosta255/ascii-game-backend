#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace hider_seeker {

// Shared request-shape rules for every hider-seeker API endpoint.

// Non-empty; letters, digits, and underscore only.
bool isValidAccount(const std::string& account);
// Non-empty; letters, digits, underscore, and hyphen only.
bool isValidMatchId(const std::string& matchId);
// 1-64 characters of letters, digits, underscore, or hyphen.
bool isValidIdempotencyKey(const std::string& key);
// A JSON integer that is not negative.
bool isValidExpectedRevision(const nlohmann::json& value);
// One of "PLAY", "QUEUE", "PASS".
bool isValidPlayMode(const std::string& playMode);
// One of the five hider-action intent names, ADD_SERVER, or REMOVE_SERVER.
bool isValidIntentType(const std::string& intentType);
// {"error": code, "message": message} - the base shape every error response
// extends with endpoint-specific extra fields.
nlohmann::json buildErrorBody(const std::string& code, const std::string& message);

} // namespace hider_seeker
