#include "MatchRepository.hpp"
#include "MatchStoreView.hpp"
#include "JsonParameters.hpp"
#include <json/json.h>
#include <sstream>

MatchRepository::MatchRepository(iStore& store)
    : store_(store) {}

bool MatchRepository::init(const Match& match, CodeEnum& error) {
    Json::Value body;
    // JsonParameters params(match, JSON_MATCH_DATASTORE);
    // match.toJson(params, body);
    return store_.save(true, error, MatchStoreView(match));
}

bool MatchRepository::save(const Match& match, CodeEnum& error) {
    Json::Value body;
    // JsonParameters params(match, JSON_MATCH_DATASTORE);
    // match.toJson(params, body);
    return store_.save(false, error, MatchStoreView(match));
}
// #include <stdexcept>

bool MatchRepository::load(const std::string& filename, CodeEnum& error, Match& output) {
    nlohmann::json loaded;
    if(!store_.load(filename, error, loaded)) {
        return false;
    }
    // throw std::runtime_error(loaded.dump());
    output = (Match)loaded.get<MatchStoreView>();
    return true;
}

bool MatchRepository::list(int& limit, int& offset, int& total, CodeEnum& error, std::function<void(const std::string&)> consumer){
    return store_.list(limit, offset, total, error, consumer);
}
