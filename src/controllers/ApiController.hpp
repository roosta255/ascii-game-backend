#pragma once
#include <drogon/HttpController.h>

class ApiController : public drogon::HttpController<ApiController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ApiController::createMatch, "/api/match", drogon::Post, drogon::Options);
    ADD_METHOD_TO(ApiController::getMatch, "/api/match/{1}", drogon::Get);
    ADD_METHOD_TO(ApiController::getMatchList, "/api/matches", drogon::Get);
    ADD_METHOD_TO(ApiController::getGeneratorList, "/api/generators", drogon::Get);
    ADD_METHOD_TO(ApiController::joinMatch, "/api/match/{1}/join", drogon::Post, drogon::Options);
    ADD_METHOD_TO(ApiController::leaveMatch, "/api/match/{1}/leave", drogon::Post, drogon::Options);
    ADD_METHOD_TO(ApiController::startMatch, "/api/match/{1}/start", drogon::Post, drogon::Options);
    ADD_METHOD_TO(ApiController::moveCharacterToDoor, "/api/match/{1}/move_character", drogon::Post, drogon::Options);
    ADD_METHOD_TO(ApiController::activateCharacter, "/api/match/{1}/activate_character", drogon::Post, drogon::Options);
    ADD_METHOD_TO(ApiController::activateLock, "/api/match/{1}/activate_lock", drogon::Post, drogon::Options);
    ADD_METHOD_TO(ApiController::endTurn, "/api/match/{1}/end_turn", drogon::Post, drogon::Options);
    METHOD_LIST_END

    void createMatch(const drogon::HttpRequestPtr &,
                     std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void getMatch(const drogon::HttpRequestPtr &,
                  std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                  std::string matchId);

    void getMatchList(const drogon::HttpRequestPtr &,
                      std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void getGeneratorList(const drogon::HttpRequestPtr &,
                          std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void joinMatch(const drogon::HttpRequestPtr &,
                   std::function<void(const drogon::HttpResponsePtr &)> &&,
                   std::string matchId);

    void leaveMatch(const drogon::HttpRequestPtr &,
                    std::function<void(const drogon::HttpResponsePtr &)> &&,
                    std::string matchId);

    void startMatch(const drogon::HttpRequestPtr &,
                    std::function<void(const drogon::HttpResponsePtr &)> &&,
                    std::string matchId);

    void moveCharacterToDoor(const drogon::HttpRequestPtr& req,
                             std::function<void (const drogon::HttpResponsePtr &)> &&callback,
                             std::string matchId);

    void activateCharacter(const drogon::HttpRequestPtr& req,
                           std::function<void (const drogon::HttpResponsePtr &)> &&callback,
                           std::string matchId);

    void activateLock(const drogon::HttpRequestPtr &req,
                     std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                     std::string matchId);

    void endTurn(const drogon::HttpRequestPtr& req,
                 std::function<void (const drogon::HttpResponsePtr &)> &&callback,
                 std::string matchId);

    static void invokeResponse(const drogon::HttpStatusCode code, const std::string& body, std::function<void (const drogon::HttpResponsePtr &)> &&callback);
    static void invokeResponse200(const std::string& body, std::function<void (const drogon::HttpResponsePtr &)> &&callback);
    static void invokeResponse400(const std::string& body, std::function<void (const drogon::HttpResponsePtr &)> &&callback);
    static void invokeResponse404(const std::string& body, std::function<void (const drogon::HttpResponsePtr &)> &&callback);
    static void invokeResponse409(const std::string& body, std::function<void (const drogon::HttpResponsePtr &)> &&callback);
    static void invokeResponse500(const std::string& body, std::function<void (const drogon::HttpResponsePtr &)> &&callback);
};
