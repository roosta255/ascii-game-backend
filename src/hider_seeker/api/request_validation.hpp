#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace hider_seeker {

bool isValidAccount(const std::string& account);
bool isValidMatchId(const std::string& matchId);
bool isValidIdempotencyKey(const std::string& key);
bool isValidExpectedRevision(const nlohmann::json& value);
bool isValidPlayMode(const std::string& playMode);
bool isValidIntentType(const std::string& intentType);
nlohmann::json buildErrorBody(int statusCode, const std::string& message);

} // namespace hider_seeker