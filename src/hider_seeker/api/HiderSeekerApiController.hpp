#pragma once

#include <drogon/HttpController.h>

namespace hider_seeker {

class HiderSeekerApiController : public drogon::HttpController<HiderSeekerApiController> {
public:
    METHOD_LIST_BEGIN
    // POST /api/v1/matches -> createMatch
    ADD_METHOD_TO(HiderSeekerApiController::createMatch, "/api/v1/matches", drogon::Post);
    // POST /api/v1/matches/{1}/join -> joinMatch
    ADD_METHOD_TO(HiderSeekerApiController::joinMatch, "/api/v1/matches/{1}/join", drogon::Post);
    // POST /api/v1/matches/{1}/start -> startMatch
    ADD_METHOD_TO(HiderSeekerApiController::startMatch, "/api/v1/matches/{1}/start", drogon::Post);
    // GET /api/v1/matches/{1} -> getMatch
    ADD_METHOD_TO(HiderSeekerApiController::getMatch, "/api/v1/matches/{1}", drogon::Get);
    // POST /api/v1/matches/{1}/intents -> submitMove
    ADD_METHOD_TO(HiderSeekerApiController::submitMove, "/api/v1/matches/{1}/intents", drogon::Post);
    // DELETE /api/v1/matches/{1}/intents/{2} -> cancelMove
    ADD_METHOD_TO(HiderSeekerApiController::cancelMove, "/api/v1/matches/{1}/intents/{2}", drogon::Delete);
    METHOD_LIST_END

    void createMatch(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void joinMatch(const drogon::HttpRequestPtr& req,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void startMatch(const drogon::HttpRequestPtr& req,
                    std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void getMatch(const drogon::HttpRequestPtr& req,
                  std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void submitMove(const drogon::HttpRequestPtr& req,
                    std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void cancelMove(const drogon::HttpRequestPtr& req,
                    std::function<void(const drogon::HttpResponsePtr&)>&& callback);

private:
    static HiderSeekerService service_;
};

} // namespace hider_seeker