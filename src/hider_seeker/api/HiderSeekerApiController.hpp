#pragma once
#include <drogon/HttpController.h>

// Thin HTTP surface of the hider-seeker move API: every handler passes the
// request body and path parameters to HiderSeekerService and returns its
// ApiResponse unchanged.
class HiderSeekerApiController : public drogon::HttpController<HiderSeekerApiController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(HiderSeekerApiController::createMatch, "/api/v1/matches", drogon::Post, drogon::Options);
    ADD_METHOD_TO(HiderSeekerApiController::joinMatch, "/api/v1/matches/{1}/join", drogon::Post, drogon::Options);
    ADD_METHOD_TO(HiderSeekerApiController::startMatch, "/api/v1/matches/{1}/start", drogon::Post, drogon::Options);
    ADD_METHOD_TO(HiderSeekerApiController::getMatch, "/api/v1/matches/{1}", drogon::Get, drogon::Options);
    ADD_METHOD_TO(HiderSeekerApiController::submitMove, "/api/v1/matches/{1}/intents", drogon::Post, drogon::Options);
    ADD_METHOD_TO(HiderSeekerApiController::cancelMove, "/api/v1/matches/{1}/intents/{2}", drogon::Delete, drogon::Options);
    ADD_METHOD_TO(HiderSeekerApiController::getLatestView, "/api/v1/matches/{1}/views/{2}/latest", drogon::Get, drogon::Options);
    ADD_METHOD_TO(HiderSeekerApiController::getRevisionView, "/api/v1/matches/{1}/views/{2}/revisions/{3}", drogon::Get, drogon::Options);
    METHOD_LIST_END

    void createMatch(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void joinMatch(const drogon::HttpRequestPtr& req,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                   std::string matchId);

    void startMatch(const drogon::HttpRequestPtr& req,
                    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                    std::string matchId);

    void getMatch(const drogon::HttpRequestPtr& req,
                  std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                  std::string matchId);

    void submitMove(const drogon::HttpRequestPtr& req,
                    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                    std::string matchId);

    void cancelMove(const drogon::HttpRequestPtr& req,
                    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                    std::string matchId,
                    std::string intentId);
    
    void getLatestView(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                       std::string matchId,
                       std::string audience);
    
    void getRevisionView(const drogon::HttpRequestPtr& req,
                         std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                         std::string matchId,
                         std::string audience,
                         std::string revision);
};
