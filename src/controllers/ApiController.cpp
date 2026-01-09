#include "ApiController.hpp"
#include "AccountController.hpp"
#include "CodeEnum.hpp"
#include "FileStore.hpp"
#include "GeneratorFlyweight.hpp"
#include "iLayout.hpp"
#include "JsonParameters.hpp"
#include "MatchController.hpp"
#include "MatchApiParameters.hpp"
#include "MatchApiView.hpp"
#include "Timestamp.hpp"
#include <json/json.h>

static FileStore matchStore("var/state/matches");
static FileStore accountStore("var/state/accounts");
static MatchController matchController(matchStore);
static AccountController accountController(accountStore);

// docker build -t drogon-server .

// docker run -p 8081:8081 drogon-server

// "C:\Program Files\Git\bin\bash.exe"

// curl -X POST http://localhost:8081/api/match \
  -H "Content-Type: application/json" \
  -d '{ "host": "host_1", "name": "m2", "generator": "TUTORIAL" }'

// curl http://localhost:8081/api/match/match_1

// curl -X POST http://localhost:8081/api/match/match_1/start

// curl -X POST http://localhost:8081/api/match/match_1/join \
  -H "Content-Type: application/json" \
  -d '{ "account": "builder_1" }'

// curl -X POST http://localhost:8081/api/match/match_1/move_character \
  -H "Content-Type: application/json" \
  -d '{ "account": "host_1", "character": 144, "room": 0, "direction": 1 }'

// curl -X POST http://localhost:8081/api/match/match_1/activate_character \
  -H "Content-Type: application/json" \
  -d '{ "account": "host_1", "character": 7448, "room": 1, "target": 144 }'

// curl -X POST http://localhost:8081/api/match/match_1/move_character \
  -H "Content-Type: application/json" \
  -d '{ "account": "host_1", "character": 144, "room": 1, "direction": 0 }'

// curl -X POST http://localhost:8081/api/match/match_1/activate_character \
  -H "Content-Type: application/json" \
  -d '{ "account": "host_1", "character": 7496, "room": 9, "target": 144 }'

// curl -X POST http://localhost:8081/api/match/match_1/move_character \
  -H "Content-Type: application/json" \
  -d '{ "account": "host_1", "character": 144, "room": 9, "direction": 1 }'

// curl -X POST http://localhost:8081/api/match/match_1/move_character \
  -H "Content-Type: application/json" \
  -d '{ "account": "host_1", "character": 144, "room": 10, "direction": 2 }'

// curl -X POST http://localhost:8081/api/match/match_1/move_character \
  -H "Content-Type: application/json" \
  -d '{ "account": "host_1", "character": 144, "room": 2, "direction": 1 }'

// curl -X POST http://localhost:8081/api/match/match_1/activate_character \
  -H "Content-Type: application/json" \
  -d '{ "account": "host_1", "character": 7472, "room": 3, "target": 144 }'

// curl -X POST http://localhost:8081/api/match/match_1/move_character \
  -H "Content-Type: application/json" \
  -d '{ "account": "host_1", "character": 144, "room": 3, "direction": 3 }'

// curl -X POST http://localhost:8081/api/match/match_1/activate_lock \
  -H "Content-Type: application/json" \
  -d '{ "account": "host_1", "character": 7472, "room": 3, "target": 144 }'

void ApiController::createMatch
( const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback )
{
    CodeEnum error = CODE_UNKNOWN_ERROR;
    Match created;
    auto json = req->getJsonObject();
    if (!json || !json->isMember("host"))
        return invokeResponse400("Missing host field", std::move(callback));
    created.host = (*json)["host"].asString();

    if (!json || !json->isMember("name"))
        return invokeResponse400("Missing name field", std::move(callback));
    created.username = (*json)["name"].asString();

    if (!json || !json->isMember("generator"))
        return invokeResponse400("Missing generator field", std::move(callback));

    const auto generatorName = (*json)["generator"].asString();
    if (!GeneratorFlyweight::indexByString(generatorName, created.generator)) {
        return invokeResponse400("Failed to parse generator: " + generatorName, std::move(callback));
    }

    created.setFilename();
    if (!created.generate(5, error))
        return invokeResponse500(code_to_message(error, "Failed to create match due to: "), std::move(callback));

    if (!matchController.init(created, error))
        return invokeResponse409(code_to_message(error, "Failed to save match due to: "), std::move(callback));

    Json::Value out;
    out["match"] = created.filename;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(out);
    return callback(resp);
}

void ApiController::getMatch
( const drogon::HttpRequestPtr &, std::function<void(const drogon::HttpResponsePtr &)> &&callback, std::string matchId )
{
    CodeEnum error = CODE_UNKNOWN_ERROR;
    Match match;
    if (!matchController.load(matchId, error, match))
        return invokeResponse404(code_to_message(error, "Failed to load match due to: "), std::move(callback));

    // try {
    //     if (!matchController.load(matchId, error, match))
    //         return invokeResponse404(code_to_message(error, "Failed to load match due to: "), std::move(callback));
    //     return invokeResponse200("Here", std::move(callback));
    // } catch (const std::exception& e) {
    //     return invokeResponse500(e.what(), std::move(callback));
    // }

    MatchApiParameters params{
        .mask = ~0x0,
        .match = match
    };
    MatchApiView view(params);
    nlohmann::json body(view);
    auto resp = drogon::HttpResponse::newHttpJsonResponse(body.dump());
    return callback(resp);
}

void ApiController::getMatchList
( const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr &)> &&callback )
{
    CodeEnum error = CODE_UNKNOWN_ERROR;

    // Parse params
    constexpr auto LIST_LIMIT = 40;
    int limit = req->getOptionalParameter<int>("limit").value_or(LIST_LIMIT);
    int offset = req->getOptionalParameter<int>("offset").value_or(0);
    int total = 0;

    // Sanitize bounds
    limit = std::clamp(limit, 1, LIST_LIMIT);
    offset = std::max(0, offset);

    Json::Value matches(Json::arrayValue);
    const bool success = matchController.list(limit, offset, total, error, [&](const std::string& matchId){
        matches.append(matchId);
    });

    if (!success) {
        return invokeResponse500(code_to_message(error, "Failed to list matches due to: "), std::move(callback));
    }

    Json::Value response;
    response["total"] = total;
    response["limit"] = limit;
    response["offset"] = offset;
    response["matches"] = matches;

    callback(drogon::HttpResponse::newHttpJsonResponse(response));
}

void ApiController::getGeneratorList
( const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr &)> &&callback )
{
    Json::Value generators(Json::arrayValue);

    for (const auto& flyweight: GeneratorFlyweight::getFlyweights()) {
        generators.append(flyweight.name);
    }

    Json::Value response;
    response["generators"] = generators;

    callback(drogon::HttpResponse::newHttpJsonResponse(response));
}

void ApiController::joinMatch
( const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback, std::string matchId )
{
    auto json = req->getJsonObject();
    if (!json || !json->isMember("account"))
        return invokeResponse400("Missing account field", std::move(callback));

    std::string builder = (*json)["account"].asString();
    CodeEnum error = CODE_UNKNOWN_ERROR;
    Match match;
    if (!matchController.load(matchId, error, match))
        return invokeResponse404(code_to_message(error, "Failed to load match due to: "), std::move(callback));

    if (!match.join(builder))
        return invokeResponse500("Failed to join", std::move(callback));

    if (!matchController.save(match, error))
        return invokeResponse409(code_to_message(error, "Failed to save match due to: "), std::move(callback));

    return invokeResponse200("Joined match", std::move(callback));
}

void ApiController::leaveMatch
( const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback, std::string matchId )
{
    auto json = req->getJsonObject();
    if (!json || !json->isMember("account"))
        return invokeResponse400("Missing account field", std::move(callback));

    std::string builder = (*json)["account"].asString();
    CodeEnum error = CODE_UNKNOWN_ERROR;
    Match match;
    if (!matchController.load(matchId, error, match))
        return invokeResponse404(code_to_message(error, "Failed to load match due to: "), std::move(callback));

    if (!match.leave(builder, error))
        return invokeResponse500("Failed to leave", std::move(callback));

    if (!matchController.save(match, error))
        return invokeResponse409(code_to_message(error, "Failed to save match due to: "), std::move(callback));

    return invokeResponse200("Left match", std::move(callback));
}

void ApiController::startMatch
( const drogon::HttpRequestPtr &, std::function<void(const drogon::HttpResponsePtr &)> &&callback, std::string matchId
)
{
    CodeEnum error = CODE_UNKNOWN_ERROR;
    Match match;
    if (!matchController.load(matchId, error, match))
        return invokeResponse404(code_to_message(error, "Failed to load match due to: "), std::move(callback));

    if (!match.start())
        return invokeResponse500("Failed to start", std::move(callback));

    if (!matchController.save(match, error))
        return invokeResponse409(code_to_message(error, "Failed to save match due to: "), std::move(callback));

    return invokeResponse200("Match started", std::move(callback));
}

void ApiController::moveCharacterToDoor
( const drogon::HttpRequestPtr& req, std::function<void (const drogon::HttpResponsePtr &)> &&callback, std::string matchId
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

    if (!json->isMember("direction") && !json->isMember("floor"))
        return invokeResponse400("Missing direction and floor field", std::move(callback));

    if (json->isMember("direction") && json->isMember("floor"))
        return invokeResponse400("Both floor and direction fields set, but must choose one", std::move(callback));

    std::string accountId = (*json)["account"].asString();
    
    int roomId = (*json)["room"].asInt();
    int characterId = (*json)["character"].asInt();

    CodeEnum error = CODE_UNKNOWN_ERROR;
    Match match;
    if (!matchController.load(matchId, error, match))
        return invokeResponse404(code_to_message(error, "Failed to load match due to: "), std::move(callback));

    Timestamp now;

    if (json->isMember("direction")) {
        Cardinal direction((*json)["direction"].asInt());
        const auto result = match.moveCharacterToWall(roomId, characterId, direction, now);
        if (CODE_SUCCESS != result)
            return invokeResponse409(std::string("Movement to wall rejected due to ") + code_to_text(result), std::move(callback));
    } else if (json->isMember("floor")) {
        int floorId = (*json)["floor"].asInt();
        if (!match.moveCharacterToFloor(roomId, characterId, floorId, now, error))
            return invokeResponse409(std::string("Movement to floor rejected due to ") + code_to_text(error), std::move(callback));
    }

    if (!matchController.save(match, error))
        return invokeResponse409(code_to_message(error, "Failed to save match due to: "), std::move(callback));

    return invokeResponse200("Character moved", std::move(callback));
}

void ApiController::activateCharacter
( const drogon::HttpRequestPtr& req, std::function<void (const drogon::HttpResponsePtr &)> &&callback, std::string matchId
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

    if (!json->isMember("target"))
        return invokeResponse400("Missing target field", std::move(callback));

    std::string accountId = (*json)["account"].asString();
    int roomId = (*json)["room"].asInt();
    int characterId = (*json)["character"].asInt();
    int targetId = (*json)["target"].asInt();

    CodeEnum error = CODE_UNKNOWN_ERROR;
    Match match;
    if (!matchController.load(matchId, error, match))
        return invokeResponse404(code_to_message(error, "Failed to load match due to: "), std::move(callback));

    CodeEnum result = CODE_PREACTIVATE;

    // TODO: Validate that account can issue character orders

    // Attempt to activate the character
    result = match.activateCharacter(accountId, characterId, roomId, targetId);
    
    if (CODE_SUCCESS != result)
        return invokeResponse409(std::string("Character activation rejected due to ") + code_to_text(result), std::move(callback));

    // Save the updated match state
    if (!matchController.save(match, error))
        return invokeResponse409(code_to_message(error, "Failed to save match due to: "), std::move(callback));

    return invokeResponse200("Character activated", std::move(callback));
}

void ApiController::activateInventoryItem
( const drogon::HttpRequestPtr& req, std::function<void (const drogon::HttpResponsePtr &)> &&callback, std::string matchId
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

    if (!json->isMember("item"))
        return invokeResponse400("Missing target field", std::move(callback));

    std::string accountId = (*json)["account"].asString();
    int roomId = (*json)["room"].asInt();
    int characterId = (*json)["character"].asInt();
    int itemId = (*json)["item"].asInt();

    CodeEnum error = CODE_UNKNOWN_ERROR;
    Match match;
    if (!matchController.load(matchId, error, match))
        return invokeResponse404(code_to_message(error, "Failed to load match due to: "), std::move(callback));

    CodeEnum result = CODE_PREACTIVATE;

    // TODO: Validate that account can issue character orders

    // Attempt to activate the character
    result = match.activateInventoryItem(accountId, characterId, roomId, itemId);
    
    if (CODE_SUCCESS != result)
        return invokeResponse409(std::string("Item activation rejected due to ") + code_to_text(result), std::move(callback));

    // Save the updated match state
    if (!matchController.save(match, error))
        return invokeResponse409(code_to_message(error, "Failed to save match due to: "), std::move(callback));

    return invokeResponse200("Item activated", std::move(callback));
}

void ApiController::activateLock
( const drogon::HttpRequestPtr& req, std::function<void (const drogon::HttpResponsePtr &)> &&callback, std::string matchId
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
    int roomId = (*json)["room"].asInt();
    int characterId = (*json)["character"].asInt();
    int direction = (*json)["direction"].asInt();

    CodeEnum error = CODE_UNKNOWN_ERROR;
    Match match;
    if (!matchController.load(matchId, error, match))
        return invokeResponse404(code_to_message(error, "Failed to load match due to: "), std::move(callback));

    // Attempt to activate the lock
    const auto result = match.activateLock(accountId, characterId, roomId, direction);
    if (CODE_SUCCESS != result)
        return invokeResponse409(std::string("Lock activation rejected due to ") + code_to_text(result), std::move(callback));

    // Save the updated match state
    if (!matchController.save(match, error))
        return invokeResponse409(code_to_message(error, "Failed to save match due to: "), std::move(callback));

    return invokeResponse200("Lock activated", std::move(callback));
}

void ApiController::endTurn
( const drogon::HttpRequestPtr& req, std::function<void (const drogon::HttpResponsePtr &)> &&callback, std::string matchId
)
{
    auto json = req->getJsonObject();

    if (!json)
        return invokeResponse400("Missing json", std::move(callback));

    if (!json->isMember("account"))
        return invokeResponse400("Missing account field", std::move(callback));

    std::string accountId = (*json)["account"].asString();

    CodeEnum error = CODE_UNKNOWN_ERROR;
    Match match;
    if (!matchController.load(matchId, error, match))
        return invokeResponse404(code_to_message(error, "Failed to load match due to: "), std::move(callback));

    // End current player's turn
    CodeEnum result = CODE_SUCCESS;
    match.endTurn(accountId, result);
    if (result != CODE_SUCCESS)
        return invokeResponse409(std::string("Turn end rejected due to ") + code_to_text(result), std::move(callback));

    // Save the updated match state
    if (!matchController.save(match, result))
        return invokeResponse409(code_to_message(result, "Failed to save match due to: "), std::move(callback));

    return invokeResponse200("Turn ended", std::move(callback));
}

void ApiController::invokeResponse
( const drogon::HttpStatusCode code, const std::string& body, std::function<void (const drogon::HttpResponsePtr &)> &&callback
) {
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(code);
    resp->setBody(body);
    resp->addHeader("Access-Control-Allow-Origin", "*");
    resp->addHeader("Access-Control-Allow-Headers", "Content-Type");
    resp->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    callback(resp);
}

void ApiController::invokeResponse200 (const std::string& body, std::function<void (const drogon::HttpResponsePtr &)> &&callback) { invokeResponse(drogon::k200OK, body, std::move(callback)); }

void ApiController::invokeResponse400 (const std::string& body, std::function<void (const drogon::HttpResponsePtr &)> &&callback) { invokeResponse(drogon::k400BadRequest, body, std::move(callback)); }

void ApiController::invokeResponse404 (const std::string& body, std::function<void (const drogon::HttpResponsePtr &)> &&callback) { invokeResponse(drogon::k404NotFound, body, std::move(callback)); }

void ApiController::invokeResponse409 (const std::string& body, std::function<void (const drogon::HttpResponsePtr &)> &&callback) { invokeResponse(drogon::k409Conflict, body, std::move(callback)); }

void ApiController::invokeResponse500 (const std::string& body, std::function<void (const drogon::HttpResponsePtr &)> &&callback) { invokeResponse(drogon::k500InternalServerError, body, std::move(callback)); }
