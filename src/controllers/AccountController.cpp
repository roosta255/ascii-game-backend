#include "AccountController.hpp"
#include "AccountView.hpp"
#include "JsonParameters.hpp"
#include "Match.hpp"
#include <sstream>

AccountController::AccountController(iStore& store)
    : store_(store) {}

bool AccountController::save(const bool isInitial, CodeEnum& error, const Account& account) {
    return store_.save(isInitial, error, AccountView::make(account));
}

bool AccountController::load(const std::string& filename, CodeEnum& error, Account& account) {
    nlohmann::json loaded;
    if(!store_.load(filename, error, loaded)) {
        return false;
    }
    account = (Account)loaded.get<AccountView>();
    return true;
}

bool AccountController::list(int& limit, int& offset, int& total, CodeEnum& error, std::function<void(const std::string&)> consumer){
    return store_.list(limit, offset, total, error, consumer);
}
