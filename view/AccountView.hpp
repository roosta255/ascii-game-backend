#pragma once

#include "Account.hpp"
#include "JsonParameters.hpp"
#include <nlohmann/json.hpp>

struct AccountView {
    std::string filename;
    std::string username;
    std::string email;
    int version;

    // Optional: Additional API-only fields can go here

    inline AccountView() = default;
    static inline AccountView make(const Account& acc) {
        return AccountView{
            .filename = acc.filename,
            .username = acc.username,
            .email = acc.email,
            .version = acc.version,
        };
    }

    inline explicit operator Account() const {
        return Account{
            .filename = this->filename,
            .username = this->username,
            .email = this->email,
            .version = this->version,
        };
    }
};

// Reflection-based JSON serialization
#include <nlohmann/json.hpp>
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AccountView, filename, username, email, version)
