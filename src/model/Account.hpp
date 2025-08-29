#pragma once

#include "Array.hpp"
#include "CodeEnum.hpp"
#include <string>
#include <nlohmann/json.hpp>

class JsonParameters;
namespace Json {
    class Value;
}

struct Account {
    std::string filename, username, email;
    int version;

    void setFilename();

    bool attachMatch(const std::string& matchId, const bool isHost, CodeEnum& error);
    bool create(const std::string& name, const std::string& email, CodeEnum& error);
};
