#include <drogon/drogon.h>
#include "ActionFlyweight.hpp"
#include "AnimationFlyweight.hpp"
#include "ApiController.hpp"
#include "DoorFlyweight.hpp"
#include "ItemFlyweight.hpp"
#include "LockFlyweight.hpp"
#include "RoleFlyweight.hpp"
#include "AccountRepository.hpp"
#include "CharacterSheetApiView.hpp"
#include "CodeEnum.hpp"
#include "Codeset.hpp"
#include "FileStore.hpp"
#include "GeneratorFlyweight.hpp"
#include "iLayout.hpp"
#include "JsonParameters.hpp"
#include "MatchController.hpp"
#include "MatchRepository.hpp"
#include "MatchApiParameters.hpp"
#include "CodesetApiView.hpp"
#include "LoggedEventApiView.hpp"
#include "MatchApiView.hpp"
#include "RuleFlyweightApiView.hpp"
#include "Preactivation.hpp"
#include "Timestamp.hpp"
#include <json/json.h>

// Accepts only alphanumeric characters and underscores.
static bool isValidInput(const std::string& s) {
    if (s.empty()) return false;
    for (unsigned char c : s)
        if (!std::isalnum(c) && c != '_') return false;
    return true;
}

// Match IDs also allow '-' as a system delimiter.
static bool isValidMatchId(const std::string& s) {
    if (s.empty()) return false;
    for (unsigned char c : s)
        if (!std::isalnum(c) && c != '_' && c != '-') return false;
    return true;
}

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
    const auto host = (*json)["host"].asString();
    if (!isValidInput(host))
        return invokeResponse400("Invalid host field", std::move(callback));
    created.host = host;

    if (!json || !json->isMember("name"))
        return invokeResponse400("Missing name field", std::move(callback));
    const auto name = (*json)["name"].asString();
    if (!isValidInput(name))
        return invokeResponse400("Invalid name field", std::move(callback));
    created.username = name;

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
    if (!isValidMatchId(matchId))
        return invokeResponse400("Invalid match id", std::move(callback));
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
        .floors = controller.getFloors(),
        .traitsComputed = controller.getTraitsComputedMap()
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

    const bool isTestMode = drogon::app().getCustomConfig().get("test_mode", false).asBool();
    for (const auto& flyweight: GeneratorFlyweight::getFlyweights()) {
        if (isTestMode || !flyweight.isTest) generators.append(flyweight.name);
    }

    Json::Value response;
    response["generators"] = generators;

    callback(drogon::HttpResponse::newHttpJsonResponse(response));
}

void ApiController::joinMatch
( const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback, std::string matchId )
{
    if (!isValidMatchId(matchId))
        return invokeResponse400("Invalid match id", std::move(callback));
    auto json = req->getJsonObject();
    if (!json || !json->isMember("account"))
        return invokeResponse400("Missing account field", std::move(callback));

    std::string builder = (*json)["account"].asString();
    if (!isValidInput(builder))
        return invokeResponse400("Invalid account field", std::move(callback));
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
    if (!isValidMatchId(matchId))
        return invokeResponse400("Invalid match id", std::move(callback));
    auto json = req->getJsonObject();
    if (!json || !json->isMember("account"))
        return invokeResponse400("Missing account field", std::move(callback));

    std::string builder = (*json)["account"].asString();
    if (!isValidInput(builder))
        return invokeResponse400("Invalid account field", std::move(callback));
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
    if (!isValidMatchId(matchId))
        return invokeResponse400("Invalid match id", std::move(callback));
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
    if (!isValidMatchId(matchId))
        return invokeResponse400("Invalid match id", std::move(callback));
    auto json = req->getJsonObject();

    if (!json)
        return invokeResponse400("Missing json", std::move(callback));

    if (!json->isMember("account"))
        return invokeResponse400("Missing account field", std::move(callback));

    std::string accountId = (*json)["account"].asString();
    if (!isValidInput(accountId))
        return invokeResponse400("Invalid account field", std::move(callback));
    CodeEnum error = CODE_UNKNOWN_ERROR;
    Match match;
    if (!matchRepository.load(matchId, error, match))
        return invokeResponse404(code_to_message(error, "Failed to load match due to: "), std::move(callback));

    if (!match.endTurn(accountId, error))
        return invokeResponse409(code_to_message(error, "End turn rejected due to: "), std::move(callback));

    if (!matchRepository.save(match, error))
        return invokeResponse409(code_to_message(error, "Failed to save match due to: "), std::move(callback));

    return invokeResponse200("Turn ended", std::move(callback));
}

void ApiController::performCharacterAction
( const drogon::HttpRequestPtr& req, std::function<void (const drogon::HttpResponsePtr &)> &&callback, std::string matchId
)
{
    if (!isValidMatchId(matchId))
        return invokeResponse400("Invalid match id", std::move(callback));
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
    if (!isValidInput(accountId))
        return invokeResponse400("Invalid account field", std::move(callback));
    
    int roomId = (*json)["room"].asInt();
    int characterId = (*json)["character"].asInt();

    Match match;
    if (codeset.addFailure(!matchRepository.load(matchId, codeset.error, match)))
        return invokeResponse404(codeset.describe("Failed to load match due to: "), std::move(callback));

    if (json->isMember("isForcedTurnEnd") && (*json)["isForcedTurnEnd"].asBool()) {
        if (codeset.addFailure(!match.endTurn(accountId, codeset.error)))
            return invokeResponse409(codeset.describe("Forced turn end rejected due to: "), std::move(callback));
    }

    MatchController controller(match, codeset);

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
            .floorId = json->isMember("floor") ? (*json)["floor"].asInt() : Maybe<int>::empty(),
        },
        .playerId = accountId,
        .sourceItemIndex = json->isMember("source_item") ? (*json)["source_item"].asInt() : json->isMember("item") ? (*json)["item"].asInt() : Maybe<int>::empty(),
        .sourceInventoryId = json->isMember("source_inventory") ? (*json)["source_inventory"].asInt() : Maybe<int>::empty(),
        .isSkippingAnimations = false,
        .isSortingState = false,
        .time = Timestamp()
    };

    // Resolve targetPreactivationEntity from the presence of JSON fields.
    // actionEnum is only used to distinguish door vs lock when a direction is present.
    if (json->isMember("direction")) {
        Cardinal direction((*json)["direction"].asInt());
        if (actionEnum == ACTION_ACTIVATE_LOCK)
            preactivation.targetPreactivationEntity = PreactivationTargetLock{ direction };
        else
            preactivation.targetPreactivationEntity = PreactivationTargetDoor{ direction };
    } else if (json->isMember("target")) {
        preactivation.targetPreactivationEntity = PreactivationTargetCharacter{ (*json)["target"].asInt() };
    } else if (json->isMember("target_item") || json->isMember("item")) {
        const int itemIdx = json->isMember("target_item") ? (*json)["target_item"].asInt()
                                                          : (*json)["item"].asInt();
        preactivation.targetPreactivationEntity = PreactivationTargetItem{ itemIdx };
    }

    // Attempt to perform the character action
    std::vector<LoggedEvent> eventLog;
    const bool activated = controller.activate(preactivation, Pointer<std::vector<LoggedEvent>>(eventLog));

    // Save the updated match state (only on success)
    if (activated && codeset.addFailure(!matchRepository.save(match, codeset.error)))
        return invokeResponse409(codeset.describe("Failed to save match due to: "), std::move(callback));

    nlohmann::json eventLogJson = nlohmann::json::array();
    for (int i = 0; i < (int)eventLog.size(); ++i) {
        eventLogJson.push_back(nlohmann::json(LoggedEventApiView(eventLog[i], i)));
    }
    nlohmann::json body = {
        {"eventLog", eventLogJson},
        {"codeset",  nlohmann::json(CodesetApiView(codeset))}
    };
    const auto status = activated ? drogon::k200OK : drogon::k409Conflict;
    return invokeResponseJson(status, body.dump(), std::move(callback));
}

void ApiController::getCharacterSheet
( const drogon::HttpRequestPtr&, std::function<void(const drogon::HttpResponsePtr &)> &&callback, std::string matchId, std::string characterIdStr
)
{
    if (!isValidMatchId(matchId))
        return invokeResponse400("Invalid match id", std::move(callback));

    int characterId = 0;
    try { characterId = std::stoi(characterIdStr); }
    catch (...) { return invokeResponse400("Invalid character id", std::move(callback)); }

    CodeEnum error = CODE_UNKNOWN_ERROR;
    Match match;
    if (!matchRepository.load(matchId, error, match))
        return invokeResponse404(code_to_message(error, "Failed to load match due to: "), std::move(callback));

    Codeset codeset;
    MatchController controller(match, codeset);

    const Character* found = nullptr;
    match.accessUsedCharacters([&](const Character& ch) {
        if (ch.characterId == characterId) found = &ch;
    });

    if (!found)
        return invokeResponse404("Character not found", std::move(callback));

    const auto computed = controller.getTraitsComputed(characterId);
    CharacterSheetApiView view(*found, computed);
    nlohmann::json body(view);
    auto resp = drogon::HttpResponse::newHttpJsonResponse(body.dump());
    return callback(resp);
}

// TODO: split up per flyweight
// TODO: flyweight provided serialization
// TODO: flyweight api response as MAP
void ApiController::getFlyweights
( const drogon::HttpRequestPtr&, std::function<void(const drogon::HttpResponsePtr &)> &&callback )
{
    Json::Value roles(Json::arrayValue);
    int roleIndex = 0;
    for (const auto& fw : RoleFlyweight::getFlyweights()) {
        Json::Value entry;
        entry["index"] = roleIndex++;
        entry["name"] = fw.name;
        roles.append(entry);
    }

    Json::Value doors(Json::arrayValue);
    int doorIndex = 0;
    for (const auto& fw : DoorFlyweight::getFlyweights()) {
        Json::Value entry;
        entry["index"] = doorIndex++;
        entry["name"] = fw.name;
        doors.append(entry);
    }

    Json::Value locks(Json::arrayValue);
    int lockIndex = 0;
    for (const auto& fw : LockFlyweight::getFlyweights()) {
        Json::Value entry;
        entry["index"] = lockIndex++;
        entry["name"] = fw.name;
        locks.append(entry);
    }

    Json::Value items(Json::arrayValue);
    int itemIndex = 0;
    for (const auto& fw : ItemFlyweight::getFlyweights()) {
        Json::Value entry;
        entry["index"] = itemIndex++;
        entry["name"] = fw.name;
        items.append(entry);
    }

    Json::Value animations(Json::arrayValue);
    int animationIndex = 0;
    for (const auto& fw : AnimationFlyweight::getFlyweights()) {
        Json::Value entry;
        bool isTranslating = false;
        bool isTransition = false;
        bool isGlyphing = false;
        bool isReversible = false;
        bool isOverlay = false;
        fw.types[ANIMATION_TYPE_IS_TRANSLATING].copy(isTranslating);
        fw.types[ANIMATION_TYPE_IS_TRANSITIONING].copy(isTransition);
        fw.types[ANIMATION_TYPE_IS_GLYPHING].copy(isGlyphing);
        fw.types[ANIMATION_TYPE_IS_REVERSIBLE].copy(isReversible);
        fw.types[ANIMATION_TYPE_IS_OVERLAY].copy(isOverlay);
        entry["index"] = animationIndex++;
        entry["name"] = fw.name;
        entry["isTranslating"] = isTranslating;
        entry["isTransition"] = isTransition;
        entry["isGlyphing"] = isGlyphing;
        entry["isReversible"] = isReversible;
        entry["isOverlay"] = isOverlay;
        animations.append(entry);
    }

    Json::Value response;
    response["roles"] = roles;
    response["doors"] = doors;
    response["locks"] = locks;
    response["items"] = items;
    response["animations"] = animations;

    callback(drogon::HttpResponse::newHttpJsonResponse(response));
}

void ApiController::getFlyweight
( const drogon::HttpRequestPtr&, std::function<void(const drogon::HttpResponsePtr &)> &&callback, std::string name )
{
    if (name == "rules") {
        nlohmann::json body = RuleFlyweightApiView::buildAll();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(body.dump());
        return callback(resp);
    }
    return invokeResponse404("Unknown flyweight: " + name, std::move(callback));
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

void ApiController::invokeResponseJson
( const drogon::HttpStatusCode code, const std::string& body, std::function<void (const drogon::HttpResponsePtr &)> &&callback
) {
    auto resp = drogon::HttpResponse::newHttpJsonResponse(body);
    resp->setStatusCode(code);
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
