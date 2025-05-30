#pragma once
#include <drogon/HttpController.h>

class MatchApiController : public drogon::HttpController<MatchApiController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(MatchApiController::createMatch, "/api/match", drogon::Post);
    ADD_METHOD_TO(MatchApiController::getMatch, "/api/match/{1}", drogon::Get);
    ADD_METHOD_TO(MatchApiController::joinMatch, "/api/match/{1}/join", drogon::Post);
    ADD_METHOD_TO(MatchApiController::startMatch, "/api/match/{1}/start", drogon::Post);
    ADD_METHOD_TO(MatchApiController::moveCharacterToDoor, "/api/match/{1}/move_character", drogon::Post);
    METHOD_LIST_END

    void createMatch(const drogon::HttpRequestPtr &,
                     std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void getMatch(const drogon::HttpRequestPtr &,
                  std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                  std::string matchId);

    void joinMatch(const drogon::HttpRequestPtr &,
                   std::function<void(const drogon::HttpResponsePtr &)> &&,
                   std::string matchId);

    void startMatch(const drogon::HttpRequestPtr &,
                    std::function<void(const drogon::HttpResponsePtr &)> &&,
                    std::string matchId);
    void moveCharacterToDoor(const drogon::HttpRequestPtr& req,
                             std::function<void (const drogon::HttpResponsePtr &)> &&callback,
                             std::string matchId);
    
    static void invokeResponse(const drogon::HttpStatusCode code, const std::string& body, std::function<void (const drogon::HttpResponsePtr &)> &&callback);
    static void invokeResponse200(const std::string& body, std::function<void (const drogon::HttpResponsePtr &)> &&callback);
    static void invokeResponse400(const std::string& body, std::function<void (const drogon::HttpResponsePtr &)> &&callback);
    static void invokeResponse404(const std::string& body, std::function<void (const drogon::HttpResponsePtr &)> &&callback);
    static void invokeResponse409(const std::string& body, std::function<void (const drogon::HttpResponsePtr &)> &&callback);
    static void invokeResponse500(const std::string& body, std::function<void (const drogon::HttpResponsePtr &)> &&callback);
};
