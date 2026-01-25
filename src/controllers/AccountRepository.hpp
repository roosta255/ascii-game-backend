#pragma once

#include "CodeEnum.hpp"
#include "iStore.hpp"
#include "Account.hpp"
#include <functional>
#include <string>

class AccountRepository {
public:
    AccountRepository(iStore& store);

    bool save(const bool isInitial, CodeEnum& error, const Account& output);

    bool load(const std::string& matchId, CodeEnum& error, Account& output);

    bool list(int& limit, int& offset, int& total, CodeEnum& error, std::function<void(const std::string&)> consumer);

private:
    iStore& store_;
};
