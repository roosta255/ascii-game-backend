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
    if (!store_.save(true, error, MatchStoreView(match))) return false;
    cache_.erase(match.filename.toString());
    return true;
}

bool MatchRepository::save(const Match& match, CodeEnum& error) {
    Json::Value body;
    // JsonParameters params(match, JSON_MATCH_DATASTORE);
    // match.toJson(params, body);
    if (!store_.save(false, error, MatchStoreView(match))) return false;
    cache_.erase(match.filename.toString());
    return true;
}
// #include <stdexcept>

bool MatchRepository::load(const std::string& filename, CodeEnum& error, Match& output) {
    if (cache_.access(filename, [&output](Match& cached) { output = cached; })) {
        return true;
    }
    nlohmann::json loaded;
    if(!store_.load(filename, error, loaded)) {
        return false;
    }
    // throw std::runtime_error(loaded.dump());
    output = (Match)loaded.get<MatchStoreView>();
    cache_.set(filename, output);
    return true;
}

bool MatchRepository::list(int& limit, int& offset, int& total, CodeEnum& error, std::function<void(const std::string&)> consumer){
    return store_.list(limit, offset, total, error, consumer);
}
