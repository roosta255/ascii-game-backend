#include "ActionFlyweight.hpp"
#include "ApiController.hpp"
#include "AccountRepository.hpp"
#include "CodeEnum.hpp"
#include "Codeset.hpp"
#include "FileStore.hpp"
#include "GeneratorFlyweight.hpp"
#include "iLayout.hpp"
#include "JsonParameters.hpp"
#include "MatchController.hpp"
#include "MatchRepository.hpp"
#include "MatchApiParameters.hpp"
#include "MatchApiView.hpp"
#include "Preactivation.hpp"
#include "Timestamp.hpp"
#include <json/json.h>

static FileStore matchStore("var/state/matches");
static FileStore accountStore("var/state/accounts");
static MatchRepository matchRepository(matchStore);
static AccountRepository accountRepository(accountStore);

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
    Codeset codeset;
    MatchController controller(created, codeset);
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
    if (codeset.addFailure(!controller.generate(19950111)))
        return invokeResponse500(codeset.describe("Failed to create match due to: "), std::move(callback));

    if (!matchRepository.init(created, error))
        return invokeResponse409(code_to_message(error, "Failed to save match due to: "), std::move(callback));

    Json::Value out;
    out["match"] = created.filename.toString();
    auto resp = drogon::HttpResponse::newHttpJsonResponse(out);
    return callback(resp);
}

void ApiController::getMatch
( const drogon::HttpRequestPtr &, std::function<void(const drogon::HttpResponsePtr &)> &&callback, std::string matchId )
{
    CodeEnum error = CODE_UNKNOWN_ERROR;
    Match match;
    if (!matchRepository.load(matchId, error, match))
        return invokeResponse404(code_to_message(error, "Failed to load match due to: "), std::move(callback));

    Codeset codeset;
    MatchController controller(match, codeset);

    // try {
    //     if (!matchRepository.load(matchId, error, match))
    //         return invokeResponse404(code_to_message(error, "Failed to load match due to: "), std::move(callback));
    //     return invokeResponse200("Here", std::move(callback));
    // } catch (const std::exception& e) {
    //     return invokeResponse500(e.what(), std::move(callback));
    // }

    MatchApiParameters params{
        .mask = ~0x0,
        .match = match,
        .doors = controller.getDoors(),
        .floors = controller.getFloors()
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
    const bool success = matchRepository.list(limit, offset, total, error, [&](const std::string& matchId){
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
    if (!matchRepository.load(matchId, error, match))
        return invokeResponse404(code_to_message(error, "Failed to load match due to: "), std::move(callback));

    if (!match.join(builder))
        return invokeResponse500("Failed to join", std::move(callback));

    if (!matchRepository.save(match, error))
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
    if (!matchRepository.load(matchId, error, match))
        return invokeResponse404(code_to_message(error, "Failed to load match due to: "), std::move(callback));

    if (!match.leave(builder, error))
        return invokeResponse500("Failed to leave", std::move(callback));

    if (!matchRepository.save(match, error))
        return invokeResponse409(code_to_message(error, "Failed to save match due to: "), std::move(callback));

    return invokeResponse200("Left match", std::move(callback));
}

void ApiController::startMatch
( const drogon::HttpRequestPtr &, std::function<void(const drogon::HttpResponsePtr &)> &&callback, std::string matchId
)
{
    CodeEnum error = CODE_UNKNOWN_ERROR;
    Match match;
    if (!matchRepository.load(matchId, error, match))
        return invokeResponse404(code_to_message(error, "Failed to load match due to: "), std::move(callback));

    if (!match.start())
        return invokeResponse500("Failed to start", std::move(callback));

    if (!matchRepository.save(match, error))
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

    (*json)["action"] = json->isMember("direction") ? action_to_text(ACTION_MOVE_TO_DOOR): action_to_text(ACTION_MOVE_TO_FLOOR);
    return performCharacterAction(req, std::move(callback), matchId);
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

    (*json)["action"] = action_to_text(ACTION_ACTIVATE_CHARACTER);
    return performCharacterAction(req, std::move(callback), matchId);
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

    if (!json->isMember("item") && !json->isMember("source_item") && !json->isMember("target_item"))
        return invokeResponse400("Missing item field", std::move(callback));

    (*json)["action"] = action_to_text(ACTION_ACTIVATE_INVENTORY_ITEM);
    return performCharacterAction(req, std::move(callback), matchId);
}

void ApiController::activateDoor
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

    (*json)["action"] = action_to_text(ACTION_ACTIVATE_DOOR);
    return performCharacterAction(req, std::move(callback), matchId);
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

    (*json)["action"] = action_to_text(ACTION_ACTIVATE_LOCK);
    return performCharacterAction(req, std::move(callback), matchId);
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

    if (!json->isMember("character")) (*json)["character"] = 0;
    if (!json->isMember("room")) (*json)["room"] = 0;
    (*json)["action"] = action_to_text(ACTION_END_TURN);
    return performCharacterAction(req, std::move(callback), matchId);
}

void ApiController::performCharacterAction
( const drogon::HttpRequestPtr& req, std::function<void (const drogon::HttpResponsePtr &)> &&callback, std::string matchId
)
{
    Codeset codeset;
    auto json = req->getJsonObject();

    if (!json)
        return invokeResponse400("Missing json", std::move(callback));

    if (!json->isMember("account"))
        return invokeResponse400("Missing account field", std::move(callback));

    if (!json->isMember("character"))
        return invokeResponse400("Missing character field", std::move(callback));

    if (!json->isMember("room"))
        return invokeResponse400("Missing room field", std::move(callback));

    std::string accountId = (*json)["account"].asString();
    
    int roomId = (*json)["room"].asInt();
    int characterId = (*json)["character"].asInt();

    Match match;
    MatchController controller(match, codeset);
    if (codeset.addFailure(!matchRepository.load(matchId, codeset.error, match)))
        return invokeResponse404(codeset.describe("Failed to load match due to: "), std::move(callback));

    if (!json->isMember("action"))
        return invokeResponse400("Missing action field", std::move(callback));
    std::string actionString = (*json)["action"].asString();
    ActionEnum actionEnum;
    if (codeset.addFailure(!ActionFlyweight::indexByString(actionString, actionEnum)))
        return invokeResponse404("Failed to parse action: " + actionString, std::move(callback));

    // TODO: Validate that account can issue character orders
    Preactivation preactivation{
        .action = {
            .type = actionEnum,
            .characterId = characterId,
            .roomId = roomId,
            .targetCharacterId = json->isMember("target") ? (*json)["target"].asInt() : Maybe<int>::empty(),
            .targetItemIndex = json->isMember("target_item") ? (*json)["target_item"].asInt() : json->isMember("item") ? (*json)["item"].asInt() : Maybe<int>::empty(),
            .targetInventoryIndex = json->isMember("target_inventory") ? (*json)["target_inventory"].asInt() : Maybe<int>::empty(),
            .direction = json->isMember("direction") ? Cardinal((*json)["direction"].asInt()) : Maybe<Cardinal>::empty(),
        },
        .playerId = accountId,
        .sourceItemIndex = json->isMember("source_item") ? (*json)["source_item"].asInt() : json->isMember("item") ? (*json)["item"].asInt() : Maybe<int>::empty(),
        .sourceInventoryId = json->isMember("source_inventory") ? (*json)["source_inventory"].asInt() : Maybe<int>::empty(),
        .isSkippingAnimations = false,
        .isSortingState = false,
        .time = Timestamp()
    };

    // Attempt to perform the character action
    if (!controller.activate(preactivation))
        return invokeResponse409(codeset.describe("Character activation rejected due to: "), std::move(callback));

    // Save the updated match state
    if (codeset.addFailure(!matchRepository.save(match, codeset.error)))
        return invokeResponse409(codeset.describe("Failed to save match due to: "), std::move(callback));

    return invokeResponse200("Character performed action", std::move(callback));
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
