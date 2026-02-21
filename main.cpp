#include <drogon/drogon.h>
#include "CORSFilter.hpp"
#include <filesystem> // For C++17 and above

int main() {
    drogon::app().loadConfigFile("config.json");

    // Apply CORS headers to all outgoing responses
    drogon::app().registerPostHandlingAdvice([](const drogon::HttpRequestPtr &, const drogon::HttpResponsePtr &resp) {
        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type");
        resp->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    });

    drogon::app().registerHandler(
        "/api/{_:.*}",
        [](const drogon::HttpRequestPtr &req,
        std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k200OK);
            resp->addHeader("Access-Control-Allow-Origin", "*");
            resp->addHeader("Access-Control-Allow-Headers", "Content-Type");
            resp->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
            callback(resp);
        },
        { drogon::Options }  // Respond only to OPTIONS
    );

    drogon::app().registerHandler("/", [](const drogon::HttpRequestPtr& req, 
        std::function<void (const drogon::HttpResponsePtr &)> &&callback) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setBody("<h1>ASCII Game Backend is LIVE</h1>");
        callback(resp);
    });

    drogon::app().run();
    return 0;
}
