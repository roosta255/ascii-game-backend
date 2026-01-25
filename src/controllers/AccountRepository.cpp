#include "AccountRepository.hpp"
#include "AccountView.hpp"
#include "JsonParameters.hpp"
#include "Match.hpp"
#include <sstream>

AccountRepository::AccountRepository(iStore& store)
    : store_(store) {}

bool AccountRepository::save(const bool isInitial, CodeEnum& error, const Account& account) {
    return store_.save(isInitial, error, AccountView::make(account));
}

bool AccountRepository::load(const std::string& filename, CodeEnum& error, Account& account) {
    nlohmann::json loaded;
    if(!store_.load(filename, error, loaded)) {
        return false;
    }
    account = (Account)loaded.get<AccountView>();
    return true;
}

bool AccountRepository::list(int& limit, int& offset, int& total, CodeEnum& error, std::function<void(const std::string&)> consumer){
    return store_.list(limit, offset, total, error, consumer);
}
