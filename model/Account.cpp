#include "Account.hpp"
#include "make_filename.hpp"
#include <cctype>
#include <json/json.h>
#include <regex>

bool isAlnumDashUnderscore(const std::string& str) {
    for (char ch : str) {
        if (!std::isalnum(static_cast<unsigned char>(ch)) && ch != '-' && ch != '_') {
            return false;
        }
    }
    return true;
}

void Account::setFilename() {
    filename = make_filename("account-" + username + "-" + email);
}

bool isValidEmail(const std::string& email) {
    static const std::regex emailRegex(R"(^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}$)");
    return std::regex_match(email, emailRegex);
}

bool Account::create(const std::string& name, const std::string& email, CodeEnum& error){
    // validations
    if (!isAlnumDashUnderscore(name)) {
        error = CODE_INVALID_ACCOUNT_NAME;
        return false;
    }
    if (!isValidEmail(email)) {
        error = CODE_INVALID_EMAIL_ADDRESS;
        return false;
    }

    // assignments
    this->username = name;
    this->email = email;
    this->version = 0;
    return true;
}

bool Account::attachMatch(const std::string& matchId, const bool isHost, CodeEnum& error)
{
    return true;
}
