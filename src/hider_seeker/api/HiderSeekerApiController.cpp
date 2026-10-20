#include <drogon/drogon.h>
#include "HiderSeekerApiController.hpp"
#include "HiderSeekerService.hpp"
#include "FileStore.hpp"
#include "StructuredLog.hpp"
#include "../seeker/baseline_policy.hpp"

using hider_seeker::ApiResponse;
using hider_seeker::HiderSeekerService;
using hider_seeker::BaselineSeekerPolicy;

static FileStore hiderSeekerStore("var/state/hider_seeker_matches");
static BaselineSeekerPolicy hiderSeekerPolicy;
static HiderSeekerService hiderSeekerService(hiderSeekerStore, hiderSeekerPolicy);

// Logs the service outcome and returns its body, status, and headers
// unchanged. A 304 carries no body.
static void respond(const RequestLog& rlog, const ApiResponse& response,
                    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    if (response.status < 400) {
        rlog.completed(response.status);
    } else {
        const std::string errorCode = response.body.value("error", std::string());
        const std::string message = response.body.value("message", std::string());
        rlog.failed(response.status, message, errorCode);
    }

    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(static_cast<drogon::HttpStatusCode>(response.status));
    resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
    resp->setBody(response.status == 304 ? std::string() : response.body.dump());
    for (const auto& header : response.headers) {
        resp->addHeader(header.first, header.second);
    }
    resp->addHeader("Access-Control-Allow-Origin", "*");
    resp->addHeader("Access-Control-Allow-Headers", "Content-Type, If-None-Match");
    resp->addHeader("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS");
    resp->addHeader("Access-Control-Expose-Headers", "ETag");
    callback(resp);
}

void HiderSeekerApiController::createMatch
( const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback )
{
    RequestLog rlog("hsCreateMatch", req);
    respond(rlog, hiderSeekerService.createMatch(std::string(req->getBody())), std::move(callback));
}

void HiderSeekerApiController::joinMatch
( const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, std::string matchId )
{
    RequestLog rlog("hsJoinMatch", req);
    respond(rlog, hiderSeekerService.joinMatch(matchId, std::string(req->getBody())), std::move(callback));
}

void HiderSeekerApiController::startMatch
( const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, std::string matchId )
{
    RequestLog rlog("hsStartMatch", req);
    respond(rlog, hiderSeekerService.startMatch(matchId, std::string(req->getBody())), std::move(callback));
}

void HiderSeekerApiController::getMatch
( const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, std::string matchId )
{
    RequestLog rlog("hsGetMatch", req);
    respond(rlog, hiderSeekerService.getMatch(matchId), std::move(callback));
}

void HiderSeekerApiController::submitMove
( const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, std::string matchId )
{
    RequestLog rlog("hsSubmitMove", req);
    respond(rlog, hiderSeekerService.submitMove(matchId, std::string(req->getBody())), std::move(callback));
}

void HiderSeekerApiController::cancelMove
( const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback,
  std::string matchId, std::string intentId )
{
    RequestLog rlog("hsCancelMove", req);
    respond(rlog, hiderSeekerService.cancelMove(matchId, intentId, std::string(req->getBody())), std::move(callback));
}

void HiderSeekerApiController::getLatestView
( const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback,
  std::string matchId, std::string audience )
{
    RequestLog rlog("hsGetLatestView", req);
    respond(rlog, hiderSeekerService.getLatestView(matchId, audience, req->getParameter("account"),
                                                   req->getHeader("if-none-match")), std::move(callback));
}

void HiderSeekerApiController::getRevisionView
( const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback,
  std::string matchId, std::string audience, std::string revision )
{
    RequestLog rlog("hsGetRevisionView", req);
    respond(rlog, hiderSeekerService.getRevisionView(matchId, audience, revision, req->getParameter("account"),
                                                     req->getHeader("if-none-match")), std::move(callback));
}
