#include "HiderSeekerApiController.hpp"
#include "MatchStore.hpp"
#include "MatchStateJson.hpp"
#include "StructuredLog.hpp"
#include <algorithm>
#include <cctype>
#include <json/json.h>

using drogon::HttpRequestPtr;
using drogon::HttpResponsePtr;
using drogon::HttpStatusCode;

namespace {

hider_seeker::MatchStore& store() {
    static hider_seeker::MatchStore instance;
    return instance;
}

// Accepts only alphanumeric characters and underscores (account names, host names).
bool isValidInput(const std::string& s) {
    if (s.empty()) return false;
    for (unsigned char c : s)
        if (!std::isalnum(c) && c != '_') return false;
    return true;
}

// Match ids also allow '-' as a system delimiter.
bool isValidMatchId(const std::string& s) {
    if (s.empty()) return false;
    for (unsigned char c : s)
        if (!std::isalnum(c) && c != '_' && c != '-') return false;
    return true;
}

void invokeError(HttpStatusCode code, const std::string& message,
                  std::function<void(const HttpResponsePtr&)>&& callback) {
    Json::Value body;
    body["error"] = message;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(body);
    resp->setStatusCode(code);
    callback(resp);
}

void invokeJson(const Json::Value& body, std::function<void(const HttpResponsePtr&)>&& callback) {
    callback(drogon::HttpResponse::newHttpJsonResponse(body));
}

HttpStatusCode statusFor(hider_seeker::MatchStoreError error) {
    switch (error) {
        case hider_seeker::MatchStoreError::NOT_FOUND:
        case hider_seeker::MatchStoreError::ACCOUNT_NOT_FOUND:
            return drogon::k404NotFound;
        case hider_seeker::MatchStoreError::INVALID_SERVER_COUNT:
            return drogon::k400BadRequest;
        case hider_seeker::MatchStoreError::MATCH_NOT_IN_LOBBY:
        case hider_seeker::MatchStoreError::MATCH_FULL:
        case hider_seeker::MatchStoreError::ACCOUNT_ALREADY_JOINED:
        case hider_seeker::MatchStoreError::NO_HIDERS:
        case hider_seeker::MatchStoreError::NO_SEEKER:
            return drogon::k409Conflict;
        case hider_seeker::MatchStoreError::NONE:
            return drogon::k200OK;
    }
    return drogon::k500InternalServerError;
}

} // namespace

void HiderSeekerApiController::createMatch
( const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback )
{
    RequestLog rlog("hiderSeekerCreateMatch", req);
    rlog.debug("http_request_body", {{"body", std::string(req->getBody())}});

    auto json = req->getJsonObject();
    if (!json || !json->isMember("host")) {
        rlog.failed(400, "Missing host field", "missing_field", {{"field", "host"}});
        return invokeError(drogon::k400BadRequest, "Missing host field", std::move(callback));
    }
    const std::string host = (*json)["host"].asString();
    if (!isValidInput(host)) {
        rlog.failed(400, "Invalid host field", "invalid_field", {{"field", "host"}});
        return invokeError(drogon::k400BadRequest, "Invalid host field", std::move(callback));
    }

    if (!json->isMember("server_count")) {
        rlog.failed(400, "Missing server_count field", "missing_field", {{"field", "server_count"}});
        return invokeError(drogon::k400BadRequest, "Missing server_count field", std::move(callback));
    }
    const int serverCountRaw = (*json)["server_count"].asInt();
    if (serverCountRaw < 1 || serverCountRaw > hider_seeker::MAX_SERVERS) {
        const std::string message = "server_count must be between 1 and " + std::to_string(hider_seeker::MAX_SERVERS);
        rlog.failed(400, message, "invalid_field", {{"field", "server_count"}, {"value", serverCountRaw}});
        return invokeError(drogon::k400BadRequest, message, std::move(callback));
    }

    const uint32_t rulesetVersion = json->isMember("ruleset_version")
        ? (*json)["ruleset_version"].asUInt() : 1;

    std::string matchId;
    hider_seeker::MatchStoreError error = hider_seeker::MatchStoreError::NONE;
    if (!store().create(host, static_cast<uint8_t>(serverCountRaw), rulesetVersion, matchId, error)) {
        rlog.failed(400, "Failed to create match", hider_seeker::toString(error));
        return invokeError(statusFor(error), "Failed to create match", std::move(callback));
    }

    Json::Value out;
    out["match"] = matchId;
    rlog.completed(200, "hider_seeker_match_created", "Match created", {{"match_id", matchId}, {"host", host}});
    invokeJson(out, std::move(callback));
}

void HiderSeekerApiController::getMatch
( const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback, std::string matchId )
{
    RequestLog rlog("hiderSeekerGetMatch", req);

    if (!isValidMatchId(matchId)) {
        rlog.failed(400, "Invalid match id", "invalid_field", {{"field", "matchId"}});
        return invokeError(drogon::k400BadRequest, "Invalid match id", std::move(callback));
    }

    hider_seeker::MatchRecord record;
    hider_seeker::MatchStoreError error = hider_seeker::MatchStoreError::NONE;
    if (!store().get(matchId, record, error)) {
        rlog.failed(404, "Failed to load match", hider_seeker::toString(error), {{"match_id", matchId}});
        return invokeError(statusFor(error), "Match not found", std::move(callback));
    }

    rlog.completed(200, "hider_seeker_match_loaded", "Match loaded", {{"match_id", matchId}});
    invokeJson(hider_seeker::matchRecordToJson(matchId, record), std::move(callback));
}

void HiderSeekerApiController::getMatchList
( const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback )
{
    RequestLog rlog("hiderSeekerGetMatchList", req);

    constexpr int LIST_LIMIT = 40;
    int limit = req->getOptionalParameter<int>("limit").value_or(LIST_LIMIT);
    int offset = req->getOptionalParameter<int>("offset").value_or(0);
    limit = std::clamp(limit, 1, LIST_LIMIT);
    offset = std::max(0, offset);

    int total = 0;
    std::vector<std::string> ids;
    store().list(limit, offset, total, ids);

    Json::Value matches(Json::arrayValue);
    for (const auto& id : ids) matches.append(id);

    Json::Value out;
    out["matches"] = matches;
    out["total"] = total;
    rlog.completed(200, "hider_seeker_match_list_loaded", "Match list loaded", {{"total", total}});
    invokeJson(out, std::move(callback));
}

void HiderSeekerApiController::joinMatch
( const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback, std::string matchId )
{
    RequestLog rlog("hiderSeekerJoinMatch", req);
    rlog.debug("http_request_body", {{"body", std::string(req->getBody())}});

    if (!isValidMatchId(matchId)) {
        rlog.failed(400, "Invalid match id", "invalid_field", {{"field", "matchId"}});
        return invokeError(drogon::k400BadRequest, "Invalid match id", std::move(callback));
    }

    auto json = req->getJsonObject();
    if (!json || !json->isMember("account")) {
        rlog.failed(400, "Missing account field", "missing_field", {{"field", "account"}});
        return invokeError(drogon::k400BadRequest, "Missing account field", std::move(callback));
    }
    const std::string account = (*json)["account"].asString();
    if (!isValidInput(account)) {
        rlog.failed(400, "Invalid account field", "invalid_field", {{"field", "account"}});
        return invokeError(drogon::k400BadRequest, "Invalid account field", std::move(callback));
    }

    if (!json->isMember("role")) {
        rlog.failed(400, "Missing role field", "missing_field", {{"field", "role"}});
        return invokeError(drogon::k400BadRequest, "Missing role field", std::move(callback));
    }
    const std::string roleName = (*json)["role"].asString();
    hider_seeker::JoinRole role;
    if (roleName == "hider") {
        role = hider_seeker::JoinRole::HIDER;
    } else if (roleName == "seeker") {
        role = hider_seeker::JoinRole::SEEKER;
    } else {
        rlog.failed(400, "role must be \"hider\" or \"seeker\"", "invalid_field", {{"field", "role"}, {"value", roleName}});
        return invokeError(drogon::k400BadRequest, "role must be \"hider\" or \"seeker\"", std::move(callback));
    }

    hider_seeker::MatchStoreError error = hider_seeker::MatchStoreError::NONE;
    if (!store().join(matchId, account, role, error)) {
        rlog.failed(409, "Failed to join match", hider_seeker::toString(error), {{"match_id", matchId}, {"account", account}});
        return invokeError(statusFor(error), std::string("Failed to join match: ") + hider_seeker::toString(error), std::move(callback));
    }

    hider_seeker::MatchRecord record;
    store().get(matchId, record, error);
    rlog.completed(200, "hider_seeker_match_joined", "Joined match", {{"match_id", matchId}, {"account", account}, {"role", roleName}});
    invokeJson(hider_seeker::matchRecordToJson(matchId, record), std::move(callback));
}

void HiderSeekerApiController::leaveMatch
( const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback, std::string matchId )
{
    RequestLog rlog("hiderSeekerLeaveMatch", req);
    rlog.debug("http_request_body", {{"body", std::string(req->getBody())}});

    if (!isValidMatchId(matchId)) {
        rlog.failed(400, "Invalid match id", "invalid_field", {{"field", "matchId"}});
        return invokeError(drogon::k400BadRequest, "Invalid match id", std::move(callback));
    }

    auto json = req->getJsonObject();
    if (!json || !json->isMember("account")) {
        rlog.failed(400, "Missing account field", "missing_field", {{"field", "account"}});
        return invokeError(drogon::k400BadRequest, "Missing account field", std::move(callback));
    }
    const std::string account = (*json)["account"].asString();
    if (!isValidInput(account)) {
        rlog.failed(400, "Invalid account field", "invalid_field", {{"field", "account"}});
        return invokeError(drogon::k400BadRequest, "Invalid account field", std::move(callback));
    }

    hider_seeker::MatchStoreError error = hider_seeker::MatchStoreError::NONE;
    if (!store().leave(matchId, account, error)) {
        rlog.failed(409, "Failed to leave match", hider_seeker::toString(error), {{"match_id", matchId}, {"account", account}});
        return invokeError(statusFor(error), std::string("Failed to leave match: ") + hider_seeker::toString(error), std::move(callback));
    }

    hider_seeker::MatchRecord record;
    store().get(matchId, record, error);
    rlog.completed(200, "hider_seeker_match_left", "Left match", {{"match_id", matchId}, {"account", account}});
    invokeJson(hider_seeker::matchRecordToJson(matchId, record), std::move(callback));
}

void HiderSeekerApiController::startMatch
( const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback, std::string matchId )
{
    RequestLog rlog("hiderSeekerStartMatch", req);

    if (!isValidMatchId(matchId)) {
        rlog.failed(400, "Invalid match id", "invalid_field", {{"field", "matchId"}});
        return invokeError(drogon::k400BadRequest, "Invalid match id", std::move(callback));
    }

    hider_seeker::MatchStoreError error = hider_seeker::MatchStoreError::NONE;
    if (!store().start(matchId, error)) {
        rlog.failed(409, "Failed to start match", hider_seeker::toString(error), {{"match_id", matchId}});
        return invokeError(statusFor(error), std::string("Failed to start match: ") + hider_seeker::toString(error), std::move(callback));
    }

    hider_seeker::MatchRecord record;
    store().get(matchId, record, error);
    rlog.completed(200, "hider_seeker_match_started", "Match started", {{"match_id", matchId}});
    invokeJson(hider_seeker::matchRecordToJson(matchId, record), std::move(callback));
}
