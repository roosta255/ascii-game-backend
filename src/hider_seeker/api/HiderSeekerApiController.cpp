#include <drogon/drogon.h>
#include "HiderSeekerApiController.hpp"
#include "HiderSeekerService.hpp"
#include "FileStore.hpp"
#include "StructuredLog.hpp"

using hider_seeker::ApiResponse;
using hider_seeker::HiderSeekerService;

static FileStore hiderSeekerStore("var/state/hider_seeker_matches");
static HiderSeekerService hiderSeekerService(hiderSeekerStore);

// Logs the service outcome and returns its body and status unchanged.
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
    resp->setBody(response.body.dump());
    resp->addHeader("Access-Control-Allow-Origin", "*");
    resp->addHeader("Access-Control-Allow-Headers", "Content-Type, If-None-Match");
    resp->addHeader("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS");
    
    // Add all custom headers
    for (const auto& header : response.headers) {
        resp->addHeader(header.first, header.second);
    }
    
    // Always add ETag to Access-Control-Expose-Headers
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
    std::string account = req->getParameter("account");
    std::string ifNoneMatch = req->getHeader("if-none-match");
    respond(rlog, hiderSeekerService.getLatestView(matchId, audience, account, ifNoneMatch), std::move(callback));
}

void HiderSeekerApiController::getRevisionView
( const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback,
  std::string matchId, std::string audience, std::string revision )
{
    RequestLog rlog("hsGetRevisionView", req);
    std::string account = req->getParameter("account");
    std::string ifNoneMatch = req->getHeader("if-none-match");
    respond(rlog, hiderSeekerService.getRevisionView(matchId, audience, revision, account, ifNoneMatch), std::move(callback));
}
