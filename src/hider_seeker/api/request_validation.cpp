#include "request_validation.hpp"
#include <regex>
#include <algorithm>

namespace hider_seeker {

bool isValidAccount(const std::string& account) {
    if (account.empty()) {
        return false;
    }
    
    // Check that it contains only letters, digits, and underscore
    return std::all_of(account.begin(), account.end(), [](char c) {
        return std::isalnum(c) || c == '_';
    });
}

bool isValidMatchId(const std::string& matchId) {
    if (matchId.empty()) {
        return false;
    }
    
    // Check that it contains only letters, digits, underscore, and hyphen
    return std::all_of(matchId.begin(), matchId.end(), [](char c) {
        return std::isalnum(c) || c == '_' || c == '-';
    });
}

bool isValidIdempotencyKey(const std::string& key) {
    if (key.empty() || key.length() > 64) {
        return false;
    }
    
    // Check that it contains only letters, digits, underscore, and hyphen
    return std::all_of(key.begin(), key.end(), [](char c) {
        return std::isalnum(c) || c == '_' || c == '-';
    });
}

bool isValidExpectedRevision(const nlohmann::json& value) {
    if (value.is_number_unsigned()) {
        return true;
    }
    return false;
}

bool isValidPlayMode(const std::string& playMode) {
    return (playMode == "PLAY" || playMode == "QUEUE" || playMode == "PASS");
}

bool isValidIntentType(const std::string& intentType) {
    static const std::vector<std::string> validTypes = {
        "CONFIGURE_SERVER", "SUPPRESS_ERROR", "BOOST_CPU", "ROTATE_ROUTE", 
        "PAY_GOAL", "ADD_SERVER", "REMOVE_SERVER"
    };
    
    return std::find(validTypes.begin(), validTypes.end(), intentType) != validTypes.end();
}

nlohmann::json buildErrorBody(int statusCode, const std::string& message) {
    nlohmann::json errorBody;
    errorBody["error"] = statusCode;
    errorBody["message"] = message;
    return errorBody;
}

} // namespace hider_seeker