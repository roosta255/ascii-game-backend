#include "HiderSeekerApiController.hpp"
#include "HiderSeekerService.hpp"
#include "../store/FileStore.hpp"
#include <drogon/HttpAppFramework.h>
#include <iostream>

namespace hider_seeker {

// Static service instance
HiderSeekerService HiderSeekerApiController::service_(FileStore("var/state/hider_seeker_matches"));

void HiderSeekerApiController::createMatch(const drogon::HttpRequestPtr& req,
                                           std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto response = service_.createMatch(req->getBody());
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k201Created);
    resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
    resp->setBody(response.body.dump());
    callback(resp);
}

void HiderSeekerApiController::joinMatch(const drogon::HttpRequestPtr& req,
                                         std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto matchId = req->getPathVariable("1");
    auto response = service_.joinMatch(matchId, req->getBody());
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(static_cast<drogon::HttpStatusCode>(response.status));
    resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
    resp->setBody(response.body.dump());
    callback(resp);
}

void HiderSeekerApiController::startMatch(const drogon::HttpRequestPtr& req,
                                          std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto matchId = req->getPathVariable("1");
    auto response = service_.startMatch(matchId, req->getBody());
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(static_cast<drogon::HttpStatusCode>(response.status));
    resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
    resp->setBody(response.body.dump());
    callback(resp);
}

void HiderSeekerApiController::getMatch(const drogon::HttpRequestPtr& req,
                                        std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto matchId = req->getPathVariable("1");
    auto response = service_.getMatch(matchId);
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(static_cast<drogon::HttpStatusCode>(response.status));
    resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
    resp->setBody(response.body.dump());
    callback(resp);
}

void HiderSeekerApiController::submitMove(const drogon::HttpRequestPtr& req,
                                          std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto matchId = req->getPathVariable("1");
    auto response = service_.submitMove(matchId, req->getBody());
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(static_cast<drogon::HttpStatusCode>(response.status));
    resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
    resp->setBody(response.body.dump());
    callback(resp);
}

void HiderSeekerApiController::cancelMove(const drogon::HttpRequestPtr& req,
                                          std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto matchId = req->getPathVariable("1");
    auto intentId = req->getPathVariable("2");
    auto response = service_.cancelMove(matchId, intentId, req->getBody());
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(static_cast<drogon::HttpStatusCode>(response.status));
    resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
    resp->setBody(response.body.dump());
    callback(resp);
}

} // namespace hider_seeker