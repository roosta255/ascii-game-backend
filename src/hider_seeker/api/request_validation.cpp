#include "request_validation.hpp"
#include "../ruleset/action_rules.hpp"
#include <cctype>

namespace hider_seeker {

namespace {

bool isWordChar(unsigned char c) {
    return std::isalnum(c) || c == '_';
}

bool isIdChar(unsigned char c) {
    return isWordChar(c) || c == '-';
}

template <typename Predicate>
bool allChars(const std::string& s, Predicate predicate) {
    for (unsigned char c : s) {
        if (!predicate(c)) return false;
    }
    return true;
}

} // namespace

bool isValidAccount(const std::string& account) {
    return !account.empty() && allChars(account, isWordChar);
}

bool isValidMatchId(const std::string& matchId) {
    return !matchId.empty() && allChars(matchId, isIdChar);
}

bool isValidIdempotencyKey(const std::string& key) {
    return !key.empty() && key.size() <= 64 && allChars(key, isIdChar);
}

bool isValidExpectedRevision(const nlohmann::json& value) {
    if (value.is_number_unsigned()) return true;
    return value.is_number_integer() && value.get<int64_t>() >= 0;
}

bool isValidPlayMode(const std::string& playMode) {
    return playMode == "PLAY" || playMode == "QUEUE" || playMode == "PASS";
}

bool isValidIntentType(const std::string& intentType) {
    HiderActionMode mode;
    return hiderActionFromName(intentType.c_str(), mode);
}



nlohmann::json buildErrorBody(const std::string& code, const std::string& message) {
    return {{"error", code}, {"message", message}};
}

} // namespace hider_seeker
