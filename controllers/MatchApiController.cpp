#include "iLayout.hpp"
#include "MatchApiController.hpp"
#include "MatchController.hpp"
#include "MatchStoreFile.hpp"
#include "MovementEnum.hpp"
#include <json/json.h>

static MatchStoreFile matchStore("var/state/");
static MatchController matchController(matchStore);

// docker build -t drogon-server .

// docker run -p 8081:8081 drogon-server

// "C:\Program Files\Git\bin\bash.exe"

// curl -X POST http://localhost:8081/api/match \
  -H "Content-Type: application/json" \
  -d '{ "host": "host_1" }'

// curl http://localhost:8081/api/match/match_1

// curl -X POST http://localhost:8081/api/match/match_1/start

// curl -X POST http://localhost:8081/api/match/match_1/move_character \
  -H "Content-Type: application/json" \
  -d '{ "account": "host_1", "character": 144, "room": 0, "direction": 1 }'

// curl -X POST http://localhost:8081/api/match/match_1/move_character \
  -H "Content-Type: application/json" \
  -d '{}'

void MatchApiController::createMatch
(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback
) {
    auto json = req->getJsonObject();
    if (!json || !json->isMember("host"))
        return invokeResponse400("Missing host field", std::move(callback));

    const auto& host = (*json)["host"].asString();

    Match created;
    if (!created.generate(5, created))
        return invokeResponse500("Failed to generate", std::move(callback));

    if (!created.create(host))
        return invokeResponse500("Failed to create", std::move(callback));

    if (!matchController.saveMatch(created))
        return invokeResponse409("Failed to save match", std::move(callback));

    Json::Value out;
    out["match"] = created.match;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(out);
    return callback(resp);
}

void MatchApiController::getMatch
(
    const drogon::HttpRequestPtr &,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback,
    std::string matchId
) {
    Match match;
    if (!matchController.loadMatch(matchId, match))
        return invokeResponse404("Match not found", std::move(callback));

    Json::Value out;
    match.toJson(out);
    auto resp = drogon::HttpResponse::newHttpJsonResponse(out);
    return callback(resp);
}

void MatchApiController::joinMatch
(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback,
    std::string matchId
) {
    auto json = req->getJsonObject();
    if (!json || !json->isMember("account"))
        return invokeResponse400("Missing account field", std::move(callback));

    std::string builder = (*json)["account"].asString();
    Match match;
    if (!matchController.loadMatch(matchId, match))
        return invokeResponse404("Match not found", std::move(callback));

    if (!match.join(builder))
        return invokeResponse500("Failed to join", std::move(callback));

    if (!matchController.saveMatch(match))
        return invokeResponse409("Failed to save match", std::move(callback));

    return invokeResponse200("Joined match", std::move(callback));
}

void MatchApiController::startMatch
(
    const drogon::HttpRequestPtr &,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback,
    std::string matchId
) {
    Match match;
    if (!matchController.loadMatch(matchId, match))
        return invokeResponse404("Match not found", std::move(callback));

    if (!match.start())
        return invokeResponse500("Failed to start", std::move(callback));

    if (!matchController.saveMatch(match))
        return invokeResponse409("Failed to save match", std::move(callback));

    return invokeResponse200("Match started", std::move(callback));
}

void MatchApiController::moveCharacterToDoor
(
    const drogon::HttpRequestPtr& req,
    std::function<void (const drogon::HttpResponsePtr &)> &&callback,
    std::string matchId
)
{
    auto json = req->getJsonObject();

    if (!json)
        return invokeResponse400("Missing json", std::move(callback));

    if (!json->isMember("account"))
        return invokeResponse400("Missing account field", std::move(callback));

    if (!json->isMember("character"))
        return invokeResponse400("Missing character field", std::move(callback));

    if (!json->isMember("room"))
        return invokeResponse400("Missing room field", std::move(callback));

    if (!json->isMember("direction"))
        return invokeResponse400("Missing direction field", std::move(callback));

    std::string accountId = (*json)["account"].asString();
    Cardinal direction((*json)["direction"].asInt());
    int roomId = (*json)["room"].asInt();
    int characterId = (*json)["character"].asInt();

    Match match;
    if (!matchController.loadMatch(matchId, match))
        return invokeResponse404("Match not found", std::move(callback));

    // TODO: validate that character can move
    // TODO: validate that account can issue character orders
    const auto result = match.moveCharacterToWall(roomId, characterId, direction);
    if (MOVEMENT_SUCCESS != result)
        return invokeResponse409(std::string("Movement rejected due to ") + movement_to_text(result), std::move(callback));

    if (!matchController.saveMatch(match))
        return invokeResponse409("Failed to save match", std::move(callback));

    return invokeResponse200("Character moved", std::move(callback));
}

void MatchApiController::invokeResponse
(
    const drogon::HttpStatusCode code,
    const std::string& body,
    std::function<void (const drogon::HttpResponsePtr &)> &&callback
) {
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(code);
    resp->setBody(body);
    callback(resp);
}

void MatchApiController::invokeResponse200 (const std::string& body, std::function<void (const drogon::HttpResponsePtr &)> &&callback) { invokeResponse(drogon::k200OK, body, std::move(callback)); }

void MatchApiController::invokeResponse400 (const std::string& body, std::function<void (const drogon::HttpResponsePtr &)> &&callback) { invokeResponse(drogon::k400BadRequest, body, std::move(callback)); }

void MatchApiController::invokeResponse404 (const std::string& body, std::function<void (const drogon::HttpResponsePtr &)> &&callback) { invokeResponse(drogon::k404NotFound, body, std::move(callback)); }

void MatchApiController::invokeResponse409 (const std::string& body, std::function<void (const drogon::HttpResponsePtr &)> &&callback) { invokeResponse(drogon::k409Conflict, body, std::move(callback)); }

void MatchApiController::invokeResponse500 (const std::string& body, std::function<void (const drogon::HttpResponsePtr &)> &&callback) { invokeResponse(drogon::k500InternalServerError, body, std::move(callback)); }
