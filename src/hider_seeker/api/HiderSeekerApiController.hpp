#pragma once

// HTTP surface for the hider-seeker game's lobby: create a match, look one up,
// list them, join/leave as a hider or the seeker, and start a match once it has
// at least one hider and a claimed seeker. All the actual bookkeeping lives in
// MatchStore; this controller only translates HTTP <-> MatchStore calls.
//
// Deliberately not here: any route that would submit an intent, resolve an
// action, advance a tick, or otherwise depend on a tick engine -- there isn't
// one yet (see src/hider_seeker/domain/state.hpp's own header comment). Adding
// those routes is a later, separate job once that engine exists.

#include <drogon/HttpController.h>

class HiderSeekerApiController : public drogon::HttpController<HiderSeekerApiController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(HiderSeekerApiController::createMatch, "/api/hider_seeker/match", drogon::Post, drogon::Options);
    ADD_METHOD_TO(HiderSeekerApiController::getMatch, "/api/hider_seeker/match/{1}", drogon::Get);
    ADD_METHOD_TO(HiderSeekerApiController::getMatchList, "/api/hider_seeker/matches", drogon::Get);
    ADD_METHOD_TO(HiderSeekerApiController::joinMatch, "/api/hider_seeker/match/{1}/join", drogon::Post, drogon::Options);
    ADD_METHOD_TO(HiderSeekerApiController::leaveMatch, "/api/hider_seeker/match/{1}/leave", drogon::Post, drogon::Options);
    ADD_METHOD_TO(HiderSeekerApiController::startMatch, "/api/hider_seeker/match/{1}/start", drogon::Post, drogon::Options);
    METHOD_LIST_END

    void createMatch(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void getMatch(const drogon::HttpRequestPtr& req,
                  std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                  std::string matchId);

    void getMatchList(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void joinMatch(const drogon::HttpRequestPtr& req,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                   std::string matchId);

    void leaveMatch(const drogon::HttpRequestPtr& req,
                    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                    std::string matchId);

    void startMatch(const drogon::HttpRequestPtr& req,
                    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                    std::string matchId);
};
